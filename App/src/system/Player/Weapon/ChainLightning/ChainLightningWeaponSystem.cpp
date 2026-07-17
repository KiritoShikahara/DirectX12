#include "apppch.h"
#include "ChainLightningWeaponSystem.h"

#include"ChainLightningRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/PlayerActionLock.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Data/Weapon/ChainLightningWeaponData.h>
#include<Scene/Game/State/GameState.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Enemy/EnemyTargetUtil.h>

#include<algorithm>

namespace ecs
{
    void ChainLightningWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発動する（PerkSelect/Result中に発動し続けないようにする）
        auto stateView = registry.view<::ecs::GameStateComponent>();
        if (stateView.begin() == stateView.end()) return;
        if (registry.get<::ecs::GameStateComponent>(*stateView.begin()).GameState != ::sys::eGameState::InGame) return;
        // 必殺技演出中は他の攻撃を発動させない(自動発動武器のためFlicker Strike中は止めない。
        // 手動攻撃のみをIsPlayerActionLocked()で止める設計。PlayerActionLock.h参照)
        if (ecs::IsPlayerUltimateActive(registry)) return;

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

                const auto* masterData = DATA_MGR(data::ChainLightningWeaponData).GetById((weapon.WeaponID + 1) * 1000 + weapon.Level);
                if (masterData == nullptr) return;

                const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
                if (ownerTransform == nullptr) return;

                // SearchRadius内に敵がいなければクールダウンを消費せず待機する
                // （対象なしで空撃ちしないため）
                std::vector<entt::entity> found;
                ::sys::PhysicsSystem::OverlapSphere(registry, ownerTransform->GetPosition(), masterData->SearchRadius, found);
                const entt::entity initialTarget = ecs::targetutil::FindNearestExcluding(registry, found, ownerTransform->GetPosition(), {});
                if (!registry.valid(initialTarget)) return;

                // 攻撃回数パーク(AttackCountUp)分だけ発動を繰り返す(同じ初撃対象から再度連鎖する)
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    Zap(registry, weapon, *masterData, initialTarget);
                }

                const auto* ownerStatus = registry.try_get<ecs::PlayerStatusComponent>(weapon.Owner);
                const float cooldownRate = ownerStatus != nullptr ? ownerStatus->Current.CooldownRate : 1.0f;
                runtime.CooldownTimer = masterData->FireInterval * cooldownRate;
            });
    }

    /// <summary>最初の対象へ命中させ、JumpRadius内の未命中の敵へ最大MaxJumps回まで跳ね移らせる</summary>
    void ChainLightningWeaponSystem::Zap(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        const data::ChainLightningWeaponData& masterData,
        entt::entity initialTarget)
    {
        // AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        float damage = masterData.Damage * atkMultiplier;

        std::vector<entt::entity> visited;
        entt::entity current = initialTarget;

        for (int jump = 0; jump <= masterData.MaxJumps; ++jump)
        {
            if (!registry.valid(current)) break;

            auto* status = registry.try_get<ecs::EnemyStatusComponent>(current);
            if (status != nullptr)
            {
                status->CurrentHp = std::max(0.0f, status->CurrentHp - damage);
            }
            visited.push_back(current);

            const auto* currentTransform = registry.try_get<ecs::Transform>(current);
            if (currentTransform != nullptr)
            {
                SpawnHitEffect(registry, currentTransform->GetPosition(), masterData);
                if (status != nullptr)
                {
                    ecs::combatutil::SpawnDamageNumber(currentTransform->GetPosition(), damage, false);
                }
            }

            damage *= masterData.DamageFalloffPerJump;

            if (jump == masterData.MaxJumps) break; // 跳躍回数の上限に達した
            if (currentTransform == nullptr) break;

            std::vector<entt::entity> candidates;
            ::sys::PhysicsSystem::OverlapSphere(registry, currentTransform->GetPosition(), masterData.JumpRadius, candidates);
            const entt::entity next = ecs::targetutil::FindNearestExcluding(registry, candidates, currentTransform->GetPosition(), visited);
            if (!registry.valid(next)) break; // 跳ね移れる未命中の敵がいない

            current = next;
        }
    }

    /// <summary>命中位置にワンショットのヒットエフェクトを再生する</summary>
    void ChainLightningWeaponSystem::SpawnHitEffect(
        entt::registry& registry,
        const DirectX::XMFLOAT3& position,
        const data::ChainLightningWeaponData& masterData)
    {
        const DirectX::XMFLOAT3 effectPos = { position.x, position.y + masterData.HeightOffset, position.z };

        // 代用素材(AttackHit.efk)は視認しづらいため、HitEffectScaleで見た目を拡大する。
        // HitEffectPathは';'区切りで複数指定可能(ecs::effectutil::PlayOneShotCombined参照)。
        ecs::effectutil::PlayOneShotCombined(masterData.HitEffectPath, effectPos, masterData.HitEffectScale);
    }
}
