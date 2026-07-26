#include "apppch.h"
#include "BoneSpearWeaponSystem.h"

#include"BoneSpearRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Weapon/Projectile/ProjectileComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Enemy/EnemyTargetUtil.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<Data/Weapon/BoneSpearWeaponData.h>

namespace ecs
{
    void BoneSpearWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発射。必殺技演出中は止める
        if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

        registry.view<ecs::WeaponComponent, ecs::BoneSpearRuntimeComponent>().each(
            [&](ecs::WeaponComponent& weapon, ecs::BoneSpearRuntimeComponent& runtime)
            {
                if (weapon.Type != ecs::eWeaponType::BoneSpear) return;
                if (!registry.valid(weapon.Owner)) return;

                if (runtime.CooldownTimer > 0.0f)
                {
                    runtime.CooldownTimer -= deltaTime;
                }
                if (runtime.CooldownTimer > 0.0f) return;

                const auto* masterData = DATA_MGR(data::BoneSpearWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
                if (masterData == nullptr) return;

                const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
                if (ownerTransform == nullptr) return;

                const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();

                // 敵がいなければクールダウンを消費せず待機する(無駄撃ち防止)
                const entt::entity target = ecs::targetutil::FindNearestInRadius(
                    registry, ownerPos, masterData->SearchRadius);
                if (!registry.valid(target)) return;

                const auto* targetTransform = registry.try_get<ecs::Transform>(target);
                if (targetTransform == nullptr) return;

                const float dx = targetTransform->GetPosition().x - ownerPos.x;
                const float dz = targetTransform->GetPosition().z - ownerPos.z;
                const float lenSq = dx * dx + dz * dz;
                DirectX::XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };
                if (lenSq > 0.0001f)
                {
                    const float invLen = 1.0f / std::sqrt(lenSq);
                    direction = { dx * invLen, 0.0f, dz * invLen };
                }

                // 攻撃回数パーク(AttackCountUp)分だけ扇状に発射する
                constexpr float kMultiShotSpreadDegrees = 6.0f;
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    const DirectX::XMFLOAT3 shotDirection = ecs::combatutil::ComputeSpreadDirection(
                        direction, i, attackCount, kMultiShotSpreadDegrees);
                    Fire(registry, weapon, shotDirection, *masterData);
                }

                runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
            });
    }

    /// <summary>最も近い敵の方向へ ProjectileComponent エンティティを1体生成する</summary>
    void BoneSpearWeaponSystem::Fire(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        const DirectX::XMFLOAT3& direction,
        const data::BoneSpearWeaponData& masterData)
    {
        const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
        if (ownerTransform == nullptr) return;

        const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();

        // プレイヤー自身のコライダーに埋まって即着弾しないよう、狙い方向へ少し離した位置から発射する
        constexpr float kSpawnOffset = 1.0f;
        const DirectX::XMFLOAT3 spawnPos =
        {
            ownerPos.x + direction.x * kSpawnOffset,
            ownerPos.y + masterData.HeightOffset,
            ownerPos.z + direction.z * kSpawnOffset,
        };

        // AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        const float damage = masterData.Damage * atkMultiplier;
        const float radius = masterData.ExplosionRadius;
        // 判定半径は見た目のradiusとは別にHitRadiusMultiplierで拡大する
        const float hitRadius = radius * masterData.HitRadiusMultiplier;

        auto& manager = ::ecs::EntityManager::Get();
        auto entity = manager.CreateEntity();

        auto& transform = manager.AddComponent<ecs::Transform>(entity);
        transform.SetPosition(spawnPos);

        manager.AddComponent<ecs::ColliderComponent>(entity, ecs::ColliderComponent::MakeSphere(0.3f));
        registry.emplace<ecs::SensorTagComponent>(entity);

        auto& rigid = manager.AddComponent<ecs::RigidBodyComponent>(entity, ecs::RigidBodyComponent::MakeKinematic());
        rigid.GravityFactor = 0.0f;

        auto& projectile = manager.AddComponent<ecs::ProjectileComponent>(entity);
        projectile.Direction = direction;
        projectile.Speed = masterData.ProjectileSpeed;
        projectile.Damage = damage;
        projectile.ExplosionRadius = hitRadius;
        projectile.VisualRadius = radius;
        projectile.ExplosionEffectPath = ecs::effectutil::ResolveEffectIds(masterData.ExplosionEffectIds);
        projectile.LifeTime = masterData.ProjectileLifeTime;
        projectile.Owner = weapon.Owner;
        projectile.PierceCount = masterData.PierceCount; // 誘導はしない(IsHoming=falseのまま)、貫通のみ設定

        const std::string projectileEffectPath = ecs::effectutil::ResolveEffectIds(masterData.ProjectileEffectIds);
        if (!projectileEffectPath.empty())
        {
            auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
            effect.Asset = graphics::EffekseerManager::Get().GetEffect(projectileEffectPath);
            effect.IsLoop = true;
            // 原点(Offset)ではなく実際の発射位置を渡す(1フレーム目の表示ズレ防止)
            effect.Effect.Play(effect.Asset, spawnPos);
            graphics::EffekseerManager::MarkSpawnHidden(effect);
        }
    }
}
