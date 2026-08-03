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
    constexpr float kEffectReferenceRadius = 2.0f;
    constexpr float kDebugWireLifetime = 0.3f;
    constexpr float kEffectForwardRatio = 0.5f;
    constexpr float kAreaSeVolume = 0.4f;
}

namespace ecs
{
    void CleaveWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない
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

                // 攻撃回数パーク分だけ発動を繰り返す
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    Swing(registry, weapon, *masterData);
                }

                runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
            });
    }

    void CleaveWeaponSystem::Swing(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        const data::CleaveWeaponData& masterData)
    {
        const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
        const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
        if (ownerTransform == nullptr || ownerAim == nullptr) return;

        PLAY_SE("Assets/Sound/SE/SE_Area.aud", false, kAreaSeVolume, false, 1);

        const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
        const DirectX::XMFLOAT3& aimDir = ownerAim->Direction;

        // AtkPowerパークの強化分をCurrent/Base比で反映する
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
            if (lenSq <= 0.0001f) continue; // 自機とほぼ同座標、正規化不能は対象外

            const float invLen = 1.0f / std::sqrt(lenSq);
            const DirectX::XMFLOAT3 toEnemyDir = { dx * invLen, 0.0f, dz * invLen };

            // 狙い方向との角度がConeAngleDegrees半角を超える敵は扇の外
            const float dot = std::clamp(aimDir.x * toEnemyDir.x + aimDir.z * toEnemyDir.z, -1.0f, 1.0f);
            const float angle = std::acos(dot);
            if (angle > coneHalfAngleRad) continue;

            if (!ecs::combatutil::ApplyDamageToEnemy(registry, entity, damage)) continue;

            // EnemyChaseSystemはEnemyKnockbackComponent保持中の敵への追従をスキップするため、安全に吹き飛ばせる
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

        // 判定射程を球形近似で可視化するため、実際の扇状範囲より広く見える点に注意。トグルONの時だけ生成する
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

        // 見た目のサイズは判定射程hitRadiusではなくradius基準に合わせる
        const float scale = radius / kEffectReferenceRadius;
        const std::string effectPath = ecs::effectutil::ResolveEffectIds(masterData.EffectIds);
        ecs::effectutil::PlayOneShotCombined(effectPath, effectPos, scale);
    }
}
