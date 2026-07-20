#include "apppch.h"
#include "CleaveWeaponSystem.h"

#include"CleaveRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/AimSysten/PlayerAimComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Enemy/Knockback/EnemyKnockbackComponent.h>
#include<Data/Weapon/CleaveWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Effect/TemporaryLifetimeComponent.h>

namespace
{
    // エフェクト素材は概ねこの半径感で作られている想定の暫定値(他の武器と同じ基準)。
    constexpr float kEffectReferenceRadius = 2.0f;

    // 判定半径可視化用ワイヤーの表示時間(秒)。EffectComponentのautoDeleteに乗らない
    // デバッグ専用エンティティのため、TemporaryLifetimeComponentで明示的に破棄する。
    constexpr float kDebugWireLifetime = 0.3f;

    // エフェクト・可視化ワイヤーを再生する前方オフセット(m)。射程の半分程度、扇の中心付近に置く
    constexpr float kEffectForwardRatio = 0.5f;
}

namespace ecs
{
    void CleaveWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない(自動発動武器のため
        // Flicker Strike中は止めない設計。ecs::weaponutil::ShouldSkipAutoWeaponUpdate参照)
        if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

        registry.view<ecs::WeaponComponent, ecs::CleaveRuntimeComponent>().each(
            [&](ecs::WeaponComponent& weapon, ecs::CleaveRuntimeComponent& runtime)
            {
                if (weapon.Type != ecs::eWeaponType::Cleave) return;
                if (!registry.valid(weapon.Owner)) return;

                if (runtime.CooldownTimer > 0.0f)
                {
                    runtime.CooldownTimer -= deltaTime;
                }
                if (runtime.CooldownTimer > 0.0f) return;

                const auto* masterData = DATA_MGR(data::CleaveWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
                if (masterData == nullptr) return;

                // 攻撃回数パーク(AttackCountUp)分だけ発動を繰り返す
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    Swing(registry, weapon, *masterData);
                }

                runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
            });
    }

    /// <summary>発動: 狙い方向の扇状範囲内にいる敵全員へダメージ・ノックバックを与える</summary>
    void CleaveWeaponSystem::Swing(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        const data::CleaveWeaponData& masterData)
    {
        const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
        const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
        if (ownerTransform == nullptr || ownerAim == nullptr) return;

        const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
        const DirectX::XMFLOAT3& aimDir = ownerAim->Direction;

        // AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        const float damage = masterData.Damage * atkMultiplier;
        const float radius = masterData.Radius;
        const float hitRadius = radius * masterData.HitRadiusMultiplier;
        const float coneHalfAngleRad = DirectX::XMConvertToRadians(masterData.ConeAngleDegrees);

        mOverlapped.clear();
        ::sys::PhysicsSystem::OverlapSphere(registry, ownerPos, hitRadius, mOverlapped);

        for (entt::entity entity : mOverlapped)
        {
            if (!registry.all_of<ecs::EnemyTag>(entity)) continue;

            const auto* enemyTransform = registry.try_get<ecs::Transform>(entity);
            if (enemyTransform == nullptr) continue;

            const DirectX::XMFLOAT3& enemyPos = enemyTransform->GetPosition();
            const float dx = enemyPos.x - ownerPos.x;
            const float dz = enemyPos.z - ownerPos.z;
            const float lenSq = dx * dx + dz * dz;
            if (lenSq <= 0.0001f) continue; // 自機とほぼ同座標(正規化不能)は対象外

            const float invLen = 1.0f / std::sqrt(lenSq);
            const DirectX::XMFLOAT3 toEnemyDir = { dx * invLen, 0.0f, dz * invLen };

            // 狙い方向との角度がConeAngleDegrees(半角)を超える敵は扇の外
            const float dot = std::clamp(aimDir.x * toEnemyDir.x + aimDir.z * toEnemyDir.z, -1.0f, 1.0f);
            const float angle = std::acos(dot);
            if (angle > coneHalfAngleRad) continue;

            if (!ecs::combatutil::ApplyDamageToEnemy(registry, entity, damage)) continue;

            // EnemyChaseSystemはEnemyKnockbackComponent保持中の敵への追従移動をスキップするため、
            // ここで速度・持続時間を設定するだけで安全に吹き飛ばせる
            auto& knockback = registry.get_or_emplace<ecs::EnemyKnockbackComponent>(entity);
            knockback.Velocity = { toEnemyDir.x * masterData.KnockbackForce, 0.0f, toEnemyDir.z * masterData.KnockbackForce };
            knockback.RemainingTime = masterData.KnockbackDuration;
        }

        const DirectX::XMFLOAT3 effectPos =
        {
            ownerPos.x + aimDir.x * radius * kEffectForwardRatio,
            ownerPos.y + masterData.HeightOffset,
            ownerPos.z + aimDir.z * radius * kEffectForwardRatio,
        };

        // 実際の判定射程(hitRadius)を可視化する（ImGui「Physics Debug」→「Show Colliders」）。
        // 球形での近似表示のため、実際の扇状範囲(ConeAngleDegrees)より広く見える点に注意。
        // トグルOFF中は描画されず無駄なため、ONの時だけ生成する。
        if (graphics::PhysicsDebugRenderer::Get().IsEnabled())
        {
            auto& manager = ::ecs::EntityManager::Get();
            auto wireEntity = manager.CreateEntity();
            auto& wireTransform = manager.AddComponent<ecs::Transform>(wireEntity);
            wireTransform.SetPosition(ownerPos);
            auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(wireEntity);
            wire.Radius = hitRadius;
            wire.Color = { 0.9f, 0.6f, 0.1f, 1.0f }; // 近接武器らしい橙
            manager.AddComponent<ecs::TemporaryLifetimeComponent>(wireEntity).RemainingTime = kDebugWireLifetime;
        }

        // 見た目のサイズは判定射程(hitRadius)ではなくradius(見た目基準)に合わせる。
        // EffectPathは';'区切りで複数指定可能(ecs::effectutil::PlayOneShotCombined参照)。
        const float scale = radius / kEffectReferenceRadius;
        ecs::effectutil::PlayOneShotCombined(masterData.EffectPath, effectPos, scale);
    }
}
