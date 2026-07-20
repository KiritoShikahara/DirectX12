#include "apppch.h"
#include "FlickerStrikeWeaponSystem.h"

#include"FlickerStrikeRuntimeComponent.h"
#include"FlickerStrikeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/AimSysten/PlayerAimComponent.h>
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<system/Player/PowerCharge/PlayerPowerChargeComponent.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<system/Enemy/EnemyTargetUtil.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Effect/TemporaryLifetimeComponent.h>

namespace
{
    // 判定範囲可視化用ワイヤーの表示時間(秒)。他の武器と同じ基準
    constexpr float kDebugWireLifetime = 0.3f;

    /// <summary>演出中にこれらの手動スキル入力があった場合、Flicker Strikeのシーケンスを打ち切る</summary>
    bool IsOtherManualSkillPressed()
    {
        auto& input = ::sys::InputManager::Get();
        return input.IsActionPressed("Attack")
            || input.IsActionPressed("Attack2")
            || input.IsActionPressed("Ultimate");
    }
}

namespace ecs
{
    void FlickerStrikeWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // 注意: この武器は意図的に ecs::combatutil::GetAttackCount() (攻撃回数パーク)を
        // 参照しない。追加ワープの回数は既にチャージ数×ChargeHitCountで管理されており、
        // 攻撃回数パークまで重ねて適用すると二重に増幅してしまうため。

        // InGame中のみ動作する（PerkSelect/Result中に発動し続けないようにする）
        if (!ecs::weaponutil::IsInGame(registry)) return;

        registry.view<ecs::WeaponComponent, ecs::FlickerStrikeRuntimeComponent>().each(
            [&](ecs::WeaponComponent& weapon, ecs::FlickerStrikeRuntimeComponent& runtime)
            {
                if (weapon.Type != ecs::eWeaponType::FlickerStrike) return;
                if (!registry.valid(weapon.Owner)) return;

                auto* flicker = registry.try_get<ecs::PlayerFlickerStrikeComponent>(weapon.Owner);
                if (flicker == nullptr) return;

                const auto* masterData = DATA_MGR(data::FlickerStrikeWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
                if (masterData == nullptr) return;

                // シーケンス中は新規発動の判定をせず、ワープの継続処理のみ行う
                if (flicker->IsActive)
                {
                    UpdateActiveSequence(registry, weapon, *flicker, *masterData, deltaTime);
                    return;
                }

                // 必殺技演出中は新規発動させない
                if (ecs::IsPlayerUltimateActive(registry)) return;

                if (runtime.CooldownTimer > 0.0f)
                {
                    runtime.CooldownTimer -= deltaTime;
                }
                if (runtime.CooldownTimer > 0.0f) return;

                const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
                if (ownerAim == nullptr || !ownerAim->WantsToFireTertiary) return;

                const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
                if (ownerTransform == nullptr) return;

                // 狙い方向(マウス座標/右スティック、他の武器と同じ基準)へ直線状に敵を探す。
                // 方向上に敵がいなければ何も起きない(クールダウン消費なし)
                const entt::entity target = PickDirectionalTarget(
                    registry, ownerTransform->GetPosition(), ownerAim->Direction,
                    masterData->InitialTargetMaxRange, masterData->InitialSearchWidth);
                if (!registry.valid(target)) return;

                StartSequence(registry, weapon, *flicker, target, *masterData);

                runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
            });
    }

    /// <summary>originからdirection方向へInitialTargetMaxRange・InitialSearchWidthの
    /// 直線範囲内にいる、最も近い敵を返す(無ければentt::null)</summary>
    entt::entity FlickerStrikeWeaponSystem::PickDirectionalTarget(
        entt::registry& registry,
        const DirectX::XMFLOAT3& origin,
        const DirectX::XMFLOAT3& direction,
        float maxRange,
        float width)
    {
        mDirectionalCandidates.clear();
        ::sys::PhysicsSystem::OverlapSphere(registry, origin, maxRange, mDirectionalCandidates);

        entt::entity nearest = entt::null;
        float nearestT = 0.0f;
        bool found = false;

        for (entt::entity entity : mDirectionalCandidates)
        {
            if (!registry.all_of<ecs::EnemyTag>(entity)) continue;

            const auto* enemyTransform = registry.try_get<ecs::Transform>(entity);
            if (enemyTransform == nullptr) continue;

            const DirectX::XMFLOAT3& enemyPos = enemyTransform->GetPosition();
            const float ex = enemyPos.x - origin.x;
            const float ez = enemyPos.z - origin.z;

            // 狙い方向への射影距離(t)。0未満(背後)・maxRangeを超える(射程外)は対象外
            const float t = ex * direction.x + ez * direction.z;
            if (t < 0.0f || t > maxRange) continue;

            // 中心線からの垂線距離。widthを超える場合は直線の外
            const float perpX = ex - direction.x * t;
            const float perpZ = ez - direction.z * t;
            const float perpDistSq = perpX * perpX + perpZ * perpZ;
            if (perpDistSq > width * width) continue;

            // 直線上で最も手前(tが小さい=最初に到達する)敵を選ぶ
            if (!found || t < nearestT)
            {
                nearestT = t;
                nearest = entity;
                found = true;
            }
        }

        return nearest;
    }

    /// <summary>シーケンスを開始する：パワーチャージを全消費し、初撃を与える</summary>
    void FlickerStrikeWeaponSystem::StartSequence(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        ecs::PlayerFlickerStrikeComponent& flicker,
        entt::entity initialTarget,
        const data::FlickerStrikeWeaponData& masterData)
    {
        auto* charge = registry.try_get<ecs::PlayerPowerChargeComponent>(weapon.Owner);
        const int chargeCount = charge != nullptr ? charge->Count : 0;
        if (charge != nullptr) charge->Count = 0; // 発動と同時に全消費する

        flicker.IsActive = true;
        flicker.RemainingHits = chargeCount * masterData.ChargeHitCount;
        flicker.WarpTimer = masterData.WarpInterval;
        flicker.CurrentTarget = entt::null;

        // 演出中は必殺技と同じく無敵化する(ワープの合間に被弾しないようにするため)
        if (auto* status = registry.try_get<ecs::PlayerStatusComponent>(weapon.Owner))
        {
            status->IsInvincible = true;
        }

        WarpAndHit(registry, weapon, initialTarget, masterData);
        flicker.CurrentTarget = initialTarget;
    }

    /// <summary>シーケンス中の毎フレーム処理：他スキル入力による中断、ワープ間隔の消化、
    /// 次の対象探索、終了判定</summary>
    void FlickerStrikeWeaponSystem::UpdateActiveSequence(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        ecs::PlayerFlickerStrikeComponent& flicker,
        const data::FlickerStrikeWeaponData& masterData,
        float deltaTime)
    {
        // 他の手動スキル入力があれば、プレイヤーの操作意思を優先してシーケンスを即座に打ち切る
        if (IsOtherManualSkillPressed())
        {
            EndSequence(registry, weapon.Owner, flicker);
            return;
        }

        if (flicker.RemainingHits <= 0)
        {
            EndSequence(registry, weapon.Owner, flicker);
            return;
        }

        flicker.WarpTimer -= deltaTime;
        if (flicker.WarpTimer > 0.0f) return;

        const auto* playerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
        if (playerTransform == nullptr)
        {
            EndSequence(registry, weapon.Owner, flicker);
            return;
        }

        mWarpCandidates.clear();
        ::sys::PhysicsSystem::OverlapSphere(registry, playerTransform->GetPosition(), masterData.WarpSearchRadius, mWarpCandidates);

        // まず直前の対象を除外して探し、他に敵がいなければ直前の対象も含めて再探索する
        // (囲まれた敵が1体しかいない状況でもチャージを無駄にしないため)
        entt::entity next = ecs::targetutil::FindNearestExcluding(
            registry, mWarpCandidates, playerTransform->GetPosition(), { flicker.CurrentTarget });
        if (!registry.valid(next))
        {
            next = ecs::targetutil::FindNearestExcluding(
                registry, mWarpCandidates, playerTransform->GetPosition(), {});
        }

        if (!registry.valid(next))
        {
            // 近くに対象がいない：残りのワープ攻撃は打ち切る(消費済みのチャージは戻らない)
            EndSequence(registry, weapon.Owner, flicker);
            return;
        }

        WarpAndHit(registry, weapon, next, masterData);
        flicker.CurrentTarget = next;
        flicker.RemainingHits -= 1;
        flicker.WarpTimer = masterData.WarpInterval;
    }

    /// <summary>対象の近くへ瞬時にワープし、ダメージ・ヒットエフェクトを与える</summary>
    void FlickerStrikeWeaponSystem::WarpAndHit(
        entt::registry& registry,
        const ecs::WeaponComponent& weapon,
        entt::entity target,
        const data::FlickerStrikeWeaponData& masterData)
    {
        auto* playerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
        const auto* targetTransform = registry.try_get<ecs::Transform>(target);
        auto* targetStatus = registry.try_get<ecs::EnemyStatusComponent>(target);
        if (playerTransform == nullptr || targetTransform == nullptr || targetStatus == nullptr) return;

        const DirectX::XMFLOAT3& playerPos = playerTransform->GetPosition();
        const DirectX::XMFLOAT3& targetPos = targetTransform->GetPosition();

        // 対象から見て現在のプレイヤー方向へTeleportOffset分だけ離れた位置へワープする
        // (対象の真上に重ならないようにする)
        const float dx = playerPos.x - targetPos.x;
        const float dz = playerPos.z - targetPos.z;
        const float lenSq = dx * dx + dz * dz;
        DirectX::XMFLOAT3 dir = { 0.0f, 0.0f, 1.0f };
        if (lenSq > 0.0001f)
        {
            const float invLen = 1.0f / std::sqrt(lenSq);
            dir = { dx * invLen, 0.0f, dz * invLen };
        }

        const DirectX::XMFLOAT3 warpPos =
        {
            targetPos.x + dir.x * masterData.TeleportOffset,
            playerPos.y,
            targetPos.z + dir.z * masterData.TeleportOffset,
        };

        // Dynamic Bodyは物理側が位置の権威のため、Transformを直接書き換えただけでは
        // 次の物理ステップでJolt側の位置に上書きされてしまう。TransformDirtyTagを付与すると
        // PhysicsSystem::SyncFromTransformがJolt側の位置も明示的に上書きしてくれる
        // (PlayerUltimateSystem::FinishAndExplodeの復帰テレポートと同じ方式)
        playerTransform->SetPosition(warpPos);
        registry.emplace_or_replace<ecs::TransformDirtyTag>(weapon.Owner);

        // AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
        const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
        const float damage = masterData.Damage * atkMultiplier;

        ecs::combatutil::ApplyDamageToEnemy(registry, target, damage);

        const DirectX::XMFLOAT3 effectPos = { targetPos.x, targetPos.y + masterData.HeightOffset, targetPos.z };
        // HitEffectPathは';'区切りで複数指定可能(ecs::effectutil::PlayOneShotCombined参照)。
        ecs::effectutil::PlayOneShotCombined(masterData.HitEffectPath, effectPos, masterData.HitEffectScale);

        // 次のワープ先探索範囲(WarpSearchRadius)を可視化する（ImGui「Physics Debug」→「Show Colliders」）。
        // トグルOFF中は描画されず無駄なため、ONの時だけ生成する。
        if (graphics::PhysicsDebugRenderer::Get().IsEnabled())
        {
            auto& manager = ::ecs::EntityManager::Get();
            auto wireEntity = manager.CreateEntity();
            auto& wireTransform = manager.AddComponent<ecs::Transform>(wireEntity);
            wireTransform.SetPosition(warpPos);
            auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(wireEntity);
            wire.Radius = masterData.WarpSearchRadius;
            wire.Color = { 1.0f, 1.0f, 0.3f, 1.0f }; // フリッカーらしい黄色
            manager.AddComponent<ecs::TemporaryLifetimeComponent>(wireEntity).RemainingTime = kDebugWireLifetime;
        }
    }

    /// <summary>シーケンスを終了する：無敵化解除、状態リセット</summary>
    void FlickerStrikeWeaponSystem::EndSequence(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerFlickerStrikeComponent& flicker)
    {
        flicker.IsActive = false;
        flicker.RemainingHits = 0;
        flicker.WarpTimer = 0.0f;
        flicker.CurrentTarget = entt::null;

        if (auto* status = registry.try_get<ecs::PlayerStatusComponent>(playerEntity))
        {
            status->IsInvincible = false;
        }
    }
}
