#include "apppch.h"
#include "VoidBeamWeaponSystem.h"

#include"VoidBeamRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Enemy/EnemyTargetUtil.h>
#include<Data/Weapon/VoidBeamWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>

namespace ecs
{
    void VoidBeamWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない(自動発動武器のため
        // Flicker Strike中は止めない設計。ecs::weaponutil::ShouldSkipAutoWeaponUpdate参照)
        if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

        registry.view<ecs::WeaponComponent, ecs::VoidBeamRuntimeComponent>().each(
            [&](ecs::WeaponComponent& weapon, ecs::VoidBeamRuntimeComponent& runtime)
            {
                if (weapon.Type != ecs::eWeaponType::VoidBeam) return;
                if (!registry.valid(weapon.Owner)) return;

                if (runtime.CooldownTimer > 0.0f)
                {
                    runtime.CooldownTimer -= deltaTime;
                }
                if (runtime.CooldownTimer > 0.0f) return;

                const auto* masterData = DATA_MGR(data::VoidBeamWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
                if (masterData == nullptr) return;

                const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
                if (ownerTransform == nullptr) return;

                const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();

                // SearchRadius内に敵がいなければクールダウンを消費せず待機する
                // （狙う相手がいない状態で空撃ちしないため、Homing/Chainと同じ方針）
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

                // 攻撃回数パーク(AttackCountUp)分だけ扇状にビームを放つ
                constexpr float kMultiBeamSpreadDegrees = 8.0f;
                const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
                for (int i = 0; i < attackCount; ++i)
                {
                    const DirectX::XMFLOAT3 shotDirection = ecs::combatutil::ComputeSpreadDirection(
                        direction, i, attackCount, kMultiBeamSpreadDegrees);
                    Fire(registry, weapon, ownerPos, shotDirection, *masterData);
                }

                runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
            });
    }

    /// <summary>directionへ向けてBeamLength・BeamWidthの直線範囲内にいる敵全員へダメージを与える</summary>
    void VoidBeamWeaponSystem::Fire(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        const DirectX::XMFLOAT3& origin,
        const DirectX::XMFLOAT3& direction,
        const data::VoidBeamWeaponData& masterData)
    {
        // AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        const float damage = masterData.Damage * atkMultiplier;

        // ビーム全体を包含する球でまず候補を集め、線分への垂線距離で直線上の敵だけに絞り込む
        // （新規の物理クエリ形状(カプセル等)を増やさず、既存のOverlapSphere+数式フィルタで完結させる）。
        mCandidates.clear();
        ::sys::PhysicsSystem::OverlapSphere(registry, origin, masterData.BeamLength, mCandidates);

        // ダメージは貫通ヒットする全員に入れるが、ヒットエフェクトはMaxHitEffects体分までしか
        // 再生しない(敵が密集していると1回のビームで数十体に同時ヒットしうるため、
        // Effekseerエフェクトの同時生成数を抑える)
        int spawnedEffects = 0;
        // ループ内で毎回ID解決しないよう、ヒットエフェクトのパスは事前に1回だけ解決しておく
        const std::string hitEffectPath = ecs::effectutil::ResolveEffectIds(masterData.HitEffectIds);

        for (entt::entity entity : mCandidates)
        {
            if (!registry.all_of<ecs::EnemyTag>(entity)) continue;

            const auto* enemyTransform = registry.try_get<ecs::Transform>(entity);
            if (enemyTransform == nullptr) continue;

            const DirectX::XMFLOAT3& enemyPos = enemyTransform->GetPosition();
            const float ex = enemyPos.x - origin.x;
            const float ez = enemyPos.z - origin.z;

            // ビーム方向への射影距離(t)。0未満(背後)・BeamLengthを超える(射程外)は対象外
            const float t = ex * direction.x + ez * direction.z;
            if (t < 0.0f || t > masterData.BeamLength) continue;

            // 中心線からの垂線距離。BeamWidthを超える場合はビームの外
            const float perpX = ex - direction.x * t;
            const float perpZ = ez - direction.z * t;
            const float perpDistSq = perpX * perpX + perpZ * perpZ;
            if (perpDistSq > masterData.BeamWidth * masterData.BeamWidth) continue;

            if (!ecs::combatutil::ApplyDamageToEnemy(registry, entity, damage)) continue;

            if (spawnedEffects >= masterData.MaxHitEffects) continue;
            ++spawnedEffects;

            const DirectX::XMFLOAT3 effectPos = { enemyPos.x, enemyPos.y + masterData.HeightOffset, enemyPos.z };
            ecs::effectutil::PlayOneShotCombined(hitEffectPath, effectPos, masterData.HitEffectScale);
        }
    }
}
