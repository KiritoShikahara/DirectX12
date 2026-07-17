#include "apppch.h"
#include "BoneSpearWeaponSystem.h"

#include"BoneSpearRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/Projectile/ProjectileComponent.h>
#include<system/Player/Weapon/Homing/HomingMissileSteeringSystem.h>
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<Data/Weapon/BoneSpearWeaponData.h>
#include<Scene/Game/State/GameState.h>

namespace ecs
{
    void BoneSpearWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発射する（PerkSelect/Result中に撃ち続けないようにする）
        auto stateView = registry.view<::ecs::GameStateComponent>();
        if (stateView.begin() == stateView.end()) return;
        if (registry.get<::ecs::GameStateComponent>(*stateView.begin()).GameState != ::sys::eGameState::InGame) return;
        // 必殺技演出中は他の攻撃を発動させない
        if (ecs::IsPlayerUltimateActive(registry)) return;

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

                const auto* masterData = DATA_MGR(data::BoneSpearWeaponData).GetById(weapon.WeaponID);
                if (masterData == nullptr) return;

                const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
                if (ownerTransform == nullptr) return;

                const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();

                // SearchRadius内に敵がいなければクールダウンを消費せず待機する
                // （対象なしで直進するだけの弾を無駄撃ちしないため、Homing Missileと同じ方針）
                const entt::entity target = ecs::HomingMissileSteeringSystem::FindNearestEnemy(
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

                Fire(registry, weapon, direction, *masterData);

                const auto* ownerStatus = registry.try_get<ecs::PlayerStatusComponent>(weapon.Owner);
                const float cooldownRate = ownerStatus != nullptr ? ownerStatus->Current.CooldownRate : 1.0f;
                runtime.CooldownTimer = masterData->FireInterval * cooldownRate;
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

        // Lv1を基準（levelIndex=0）に、レベル毎の成長量を加算する。
        // AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
        const int levelIndex = std::max(0, weapon.Level - 1);
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        const float damage = (masterData.BaseDamage + masterData.DamagePerLevel * static_cast<float>(levelIndex)) * atkMultiplier;
        const float radius = masterData.BaseExplosionRadius + masterData.ExplosionRadiusPerLevel * static_cast<float>(levelIndex);
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
        projectile.PierceCount = masterData.PierceCount; // 誘導はしない(IsHoming=falseのまま)、貫通のみ設定

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
