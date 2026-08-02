#include "apppch.h"
#include "NovaWeaponSystem.h"

#include"NovaWeaponRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<Data/Weapon/NovaWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Effect/TemporaryLifetimeComponent.h>

namespace
{
    constexpr float kEffectReferenceRadius = 2.0f;
    constexpr float kDebugWireLifetime = 0.3f;
    constexpr float kAreaSeVolume = 0.4f;
}

namespace ecs
{
    void NovaWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない
        if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

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

                const auto* masterData = DATA_MGR(data::NovaWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
                if (masterData == nullptr) return;

                // 攻撃回数パーク分だけ発動を繰り返す
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    Pulse(registry, weapon, *masterData);
                }

                runtime.CooldownTimer = masterData->PulseInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
            });
    }

    void NovaWeaponSystem::Pulse(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        const data::NovaWeaponData& masterData)
    {
        const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
        if (ownerTransform == nullptr) return;

        PLAY_SE("Assets/Sound/SE/SE_Area.aud", false, kAreaSeVolume, false);

        const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
        const DirectX::XMFLOAT3 center = { ownerPos.x, ownerPos.y + masterData.HeightOffset, ownerPos.z };

        // AtkPowerパークの強化分をCurrent/Base比で反映する
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        const float damage = masterData.Damage * atkMultiplier;
        const float radius = masterData.Radius;
        const float hitRadius = radius * masterData.HitRadiusMultiplier;

        mOverlapped.clear();
        ::sys::PhysicsSystem::OverlapSphere(registry, center, hitRadius, mOverlapped);

        for (entt::entity entity : mOverlapped)
        {
            ecs::combatutil::ApplyDamageToEnemy(registry, entity, damage);
        }

        // 実際の判定半径を可視化する、付け忘れると永久に残り続けるためTemporaryLifetimeComponentで明示的に破棄する
        if (graphics::PhysicsDebugRenderer::Get().IsEnabled())
        {
            auto& manager = ::ecs::EntityManager::Get();
            auto wireEntity = manager.CreateEntity();
            auto& transform = manager.AddComponent<ecs::Transform>(wireEntity);
            transform.SetPosition(center);
            auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(wireEntity);
            wire.Radius = hitRadius;
            wire.Color = { 0.9f, 0.3f, 1.0f, 1.0f }; // Novaらしい紫
            manager.AddComponent<ecs::TemporaryLifetimeComponent>(wireEntity).RemainingTime = kDebugWireLifetime;
        }

        // 見た目のサイズは判定半径hitRadiusではなくradius基準に合わせる
        const float scale = radius / kEffectReferenceRadius;
        const std::string effectPath = ecs::effectutil::ResolveEffectIds(masterData.EffectIds);
        ecs::effectutil::PlayOneShotCombined(effectPath, center, scale);
    }
}
