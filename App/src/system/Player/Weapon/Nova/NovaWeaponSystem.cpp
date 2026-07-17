#include "apppch.h"
#include "NovaWeaponSystem.h"

#include"NovaWeaponRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/PlayerActionLock.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Data/Weapon/NovaWeaponData.h>
#include<Scene/Game/State/GameState.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
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
}

namespace ecs
{
    void NovaWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発動する（PerkSelect/Result中に発動し続けないようにする）
        auto stateView = registry.view<::ecs::GameStateComponent>();
        if (stateView.begin() == stateView.end()) return;
        if (registry.get<::ecs::GameStateComponent>(*stateView.begin()).GameState != ::sys::eGameState::InGame) return;
        // 必殺技演出中は他の攻撃を発動させない(自動発動武器のためFlicker Strike中は止めない。
        // 手動攻撃のみをIsPlayerActionLocked()で止める設計。PlayerActionLock.h参照)
        if (ecs::IsPlayerUltimateActive(registry)) return;

        registry.view<ecs::WeaponComponent, ecs::NovaWeaponRuntimeComponent>().each(
            [&](ecs::WeaponComponent& weapon, ecs::NovaWeaponRuntimeComponent& runtime)
            {
                if (weapon.Type != ecs::eWeaponType::Nova) return;
                if (!registry.valid(weapon.Owner)) return;

                if (runtime.CooldownTimer > 0.0f)
                {
                    runtime.CooldownTimer -= deltaTime;
                }
                if (runtime.CooldownTimer > 0.0f) return;

                const auto* masterData = DATA_MGR(data::NovaWeaponData).GetById((weapon.WeaponID + 1) * 1000 + weapon.Level);
                if (masterData == nullptr) return;

                // 攻撃回数パーク(AttackCountUp)分だけ発動を繰り返す
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    Pulse(registry, weapon, *masterData);
                }

                const auto* ownerStatus = registry.try_get<ecs::PlayerStatusComponent>(weapon.Owner);
                const float cooldownRate = ownerStatus != nullptr ? ownerStatus->Current.CooldownRate : 1.0f;
                runtime.CooldownTimer = masterData->PulseInterval * cooldownRate;
            });
    }

    /// <summary>発動: 所有者中心に球形ダメージを与え、ワンショットエフェクトを再生する</summary>
    void NovaWeaponSystem::Pulse(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        const data::NovaWeaponData& masterData)
    {
        const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
        if (ownerTransform == nullptr) return;

        const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
        const DirectX::XMFLOAT3 center = { ownerPos.x, ownerPos.y + masterData.HeightOffset, ownerPos.z };

        // AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        const float damage = masterData.Damage * atkMultiplier;
        const float radius = masterData.Radius;
        const float hitRadius = radius * masterData.HitRadiusMultiplier;

        std::vector<entt::entity> overlapped;
        ::sys::PhysicsSystem::OverlapSphere(registry, center, hitRadius, overlapped);

        for (entt::entity entity : overlapped)
        {
            if (!registry.all_of<ecs::EnemyTag>(entity)) continue;

            auto* status = registry.try_get<ecs::EnemyStatusComponent>(entity);
            if (status == nullptr) continue;

            // ノックバック等の物理的な反応はさせず、HPのみ減少させる
            // （HPが0以下になった後の破棄は EnemyDeathSystem が担当する）
            status->CurrentHp = std::max(0.0f, status->CurrentHp - damage);

            if (const auto* enemyTransform = registry.try_get<ecs::Transform>(entity))
            {
                ecs::combatutil::SpawnDamageNumber(enemyTransform->GetPosition(), damage, false);
            }
        }

        // 実際の判定半径(hitRadius)を可視化する（ImGui「Physics Debug」→「Show Colliders」）。
        // EffectComponentのautoDeleteに乗らないため、TemporaryLifetimeComponentで
        // 明示的に一定時間後に破棄する（付け忘れると永久に残り続けるバグになる）。
        auto& manager = ::ecs::EntityManager::Get();
        auto wireEntity = manager.CreateEntity();
        auto& transform = manager.AddComponent<ecs::Transform>(wireEntity);
        transform.SetPosition(center);
        auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(wireEntity);
        wire.Radius = hitRadius;
        wire.Color = { 0.9f, 0.3f, 1.0f, 1.0f }; // Novaらしい紫
        manager.AddComponent<ecs::TemporaryLifetimeComponent>(wireEntity).RemainingTime = kDebugWireLifetime;

        // 見た目のサイズは判定半径(hitRadius)ではなくradius(見た目基準)に合わせる。
        // EffectPathは';'区切りで複数指定可能(ecs::effectutil::PlayOneShotCombined参照)。
        const float scale = radius / kEffectReferenceRadius;
        ecs::effectutil::PlayOneShotCombined(masterData.EffectPath, center, scale);
    }
}
