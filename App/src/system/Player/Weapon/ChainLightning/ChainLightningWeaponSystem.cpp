#include "apppch.h"
#include "ChainLightningWeaponSystem.h"

#include"ChainLightningRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<Data/Weapon/ChainLightningWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Enemy/EnemyTargetUtil.h>

#include<algorithm>

namespace ecs
{
    void ChainLightningWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発動。必殺技演出中は止める
        if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

        registry.view<ecs::WeaponComponent, ecs::ChainLightningRuntimeComponent>().each(
            [&](ecs::WeaponComponent& weapon, ecs::ChainLightningRuntimeComponent& runtime)
            {
                if (weapon.Type != ecs::eWeaponType::Chain) return;
                if (!registry.valid(weapon.Owner)) return;

                if (runtime.CooldownTimer > 0.0f)
                {
                    runtime.CooldownTimer -= deltaTime;
                }
                if (runtime.CooldownTimer > 0.0f) return;

                const auto* masterData = DATA_MGR(data::ChainLightningWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
                if (masterData == nullptr) return;

                const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
                if (ownerTransform == nullptr) return;

                // 敵がいなければクールダウンを消費せず待機する
                mFound.clear();
                ::sys::PhysicsSystem::OverlapSphere(registry, ownerTransform->GetPosition(), masterData->SearchRadius, mFound);
                const entt::entity initialTarget = ecs::targetutil::FindNearestExcluding(registry, mFound, ownerTransform->GetPosition(), {});
                if (!registry.valid(initialTarget)) return;

                // 攻撃回数パーク分だけ発動を繰り返す、同じ初撃対象から再度連鎖する
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    Zap(registry, weapon, *masterData, initialTarget);
                }

                runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
            });
    }

    void ChainLightningWeaponSystem::Zap(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        const data::ChainLightningWeaponData& masterData,
        entt::entity initialTarget)
    {
        // AtkPowerパークの強化分をCurrent/Base比で反映する
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        float damage = masterData.Damage * atkMultiplier;

        mVisited.clear();
        entt::entity current = initialTarget;

        for (int jump = 0; jump <= masterData.MaxJumps; ++jump)
        {
            if (!registry.valid(current)) break;

            ecs::combatutil::ApplyDamageToEnemy(registry, current, damage);
            mVisited.push_back(current);

            const auto* currentTransform = registry.try_get<ecs::Transform>(current);
            if (currentTransform != nullptr)
            {
                SpawnHitEffect(registry, currentTransform->GetPosition(), masterData);
            }

            damage *= masterData.DamageFalloffPerJump;

            if (jump == masterData.MaxJumps) break; // 跳躍回数の上限に達した
            if (currentTransform == nullptr) break;

            mCandidates.clear();
            ::sys::PhysicsSystem::OverlapSphere(registry, currentTransform->GetPosition(), masterData.JumpRadius, mCandidates);
            const entt::entity next = ecs::targetutil::FindNearestExcluding(registry, mCandidates, currentTransform->GetPosition(), mVisited);
            if (!registry.valid(next)) break; // 跳ね移れる未命中の敵がいない

            current = next;
        }
    }

    void ChainLightningWeaponSystem::SpawnHitEffect(
        entt::registry& registry,
        const DirectX::XMFLOAT3& position,
        const data::ChainLightningWeaponData& masterData)
    {
        const DirectX::XMFLOAT3 effectPos = { position.x, position.y + masterData.HeightOffset, position.z };

        const std::string hitEffectPath = ecs::effectutil::ResolveEffectIds(masterData.HitEffectIds);
        ecs::effectutil::PlayOneShotCombined(hitEffectPath, effectPos, masterData.HitEffectScale);
    }
}
