#include "apppch.h"
#include "HomingMissileWeaponSystem.h"

#include"HomingMissileRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Weapon/Projectile/ProjectileComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Enemy/EnemyTargetUtil.h>
#include<Data/Weapon/HomingMissileWeaponData.h>

namespace ecs
{
    void HomingMissileWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない(自動発動武器のため
        // Flicker Strike中は止めない設計。ecs::weaponutil::ShouldSkipAutoWeaponUpdate参照)
        if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

        registry.view<ecs::WeaponComponent, ecs::HomingMissileRuntimeComponent>().each(
            [&](ecs::WeaponComponent& weapon, ecs::HomingMissileRuntimeComponent& runtime)
            {
                if (weapon.Type != ecs::eWeaponType::Homing) return;
                if (!registry.valid(weapon.Owner)) return;

                if (runtime.CooldownTimer > 0.0f)
                {
                    runtime.CooldownTimer -= deltaTime;
                }
                if (runtime.CooldownTimer > 0.0f) return;

                const auto* masterData = DATA_MGR(data::HomingMissileWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
                if (masterData == nullptr) return;

                const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
                if (ownerTransform == nullptr) return;

                // SearchRadius内に敵がいなければクールダウンを消費せず待機する
                // （対象なしで直進するだけの弾を無駄撃ちしないため）
                const entt::entity target = ecs::targetutil::FindNearestInRadius(
                    registry, ownerTransform->GetPosition(), masterData->SearchRadius);
                if (!registry.valid(target)) return;

                // 攻撃回数パーク(AttackCountUp)分だけ扇状に発射する(全弾同じ対象を狙うが、
                // 見失った場合はHomingMissileSteeringSystemが個別に再捕捉する)
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    Fire(registry, weapon, target, *masterData, i, attackCount);
                }

                runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
            });
    }

    namespace
    {
        // 攻撃回数パークで複数発射する際の、1ショットあたりの扇状スプレッド角度(度)
        constexpr float kMultiShotSpreadDegrees = 8.0f;
    }

    /// <summary>狙い方向(shotCount>1の場合は扇状に広げたshotIndex番目の方向)へ
    /// 追尾弾(ProjectileComponent、IsHoming=true)を1体生成する</summary>
    void HomingMissileWeaponSystem::Fire(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        entt::entity target,
        const data::HomingMissileWeaponData& masterData,
        int shotIndex,
        int shotCount)
    {
        const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
        const auto* targetTransform = registry.try_get<ecs::Transform>(target);
        if (ownerTransform == nullptr || targetTransform == nullptr) return;

        const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();

        // 初速の向きは対象方向（狙いを必要としない自動発動のため）
        const float dx = targetTransform->GetPosition().x - ownerPos.x;
        const float dz = targetTransform->GetPosition().z - ownerPos.z;
        const float lenSq = dx * dx + dz * dz;
        DirectX::XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };
        if (lenSq > 0.0001f)
        {
            const float invLen = 1.0f / std::sqrt(lenSq);
            direction = { dx * invLen, 0.0f, dz * invLen };
        }
        direction = ecs::combatutil::ComputeSpreadDirection(direction, shotIndex, shotCount, kMultiShotSpreadDegrees);

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
        // 当たり判定半径は見た目基準半径(radius)とは別にHitRadiusMultiplierで拡大する。
        // エフェクトの見た目サイズは従来通りradius基準のままにするため、ここで分離する。
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
        projectile.ExplosionEffectPath = masterData.ExplosionEffectPath;
        projectile.LifeTime = masterData.ProjectileLifeTime;
        projectile.Owner = weapon.Owner;
        projectile.IsHoming = true;
        projectile.Target = target;
        projectile.TurnSpeed = masterData.TurnSpeed;
        projectile.HomingSearchRadius = masterData.SearchRadius;

        if (!masterData.ProjectileEffectPath.empty())
        {
            auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
            effect.Asset = graphics::EffekseerManager::Get().GetEffect(masterData.ProjectileEffectPath);
            effect.IsLoop = true;
            // effect.Offset(常に原点)ではなく実際の発射位置を渡す。
            // ここを Offset のまま渡すと、次フレームの EffekseerManager::Update による
            // Transform追従が効くまでの1フレームだけ原点に表示されてしまう。
            effect.Effect.Play(effect.Asset, spawnPos);
        }
    }
}
