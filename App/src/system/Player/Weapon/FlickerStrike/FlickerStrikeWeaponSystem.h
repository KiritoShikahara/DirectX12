#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct FlickerStrikeWeaponData; }

namespace ecs
{
    struct WeaponComponent;
    struct PlayerFlickerStrikeComponent;

    /// <summary>
    /// FlickerStrike型武器(WeaponComponent::Type == FlickerStrike)の発動ロジック。
    /// 狙い方向(PlayerAimComponent::Direction)へ直線状に敵を探し、最初に見つかった敵へ
    /// ワープして初撃を与える(方向上に敵がいなければ何も起きない)。命中した場合のみ、
    /// 所持しているパワーチャージ(PlayerPowerChargeComponent)を全消費し、チャージ1個につき
    /// ChargeHitCount回の追加ワープ攻撃を近くの敵へ次々行う。ワープシーケンス自体の状態は
    /// プレイヤー側のPlayerFlickerStrikeComponentが保持し、演出中は
    /// ecs::IsPlayerActionLocked()経由で他の武器・プレイヤー操作を一時停止させる
    /// (PlayerUltimateSystemと同じ設計方針)。ただし演出中に他の手動スキル(Attack/Attack2/
    /// Ultimate)の入力があった場合は、プレイヤーの操作意思を優先してシーケンスを打ち切る。
    /// </summary>
    class FlickerStrikeWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>originからdirection方向へInitialTargetMaxRange・InitialSearchWidthの
        /// 直線範囲内にいる、最も近い敵を返す(無ければentt::null)</summary>
        entt::entity PickDirectionalTarget(
            entt::registry& registry,
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& direction,
            float maxRange,
            float width);

        /// <summary>シーケンスを開始する：パワーチャージを全消費し、初撃を与える</summary>
        static void StartSequence(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            ecs::PlayerFlickerStrikeComponent& flicker,
            entt::entity initialTarget,
            const data::FlickerStrikeWeaponData& masterData);

        /// <summary>シーケンス中の毎フレーム処理：他スキル入力による中断、ワープ間隔の消化、
        /// 次の対象探索、終了判定</summary>
        void UpdateActiveSequence(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            ecs::PlayerFlickerStrikeComponent& flicker,
            const data::FlickerStrikeWeaponData& masterData,
            float deltaTime);

        /// <summary>対象の近くへ瞬時にワープし、ダメージ・ヒットエフェクトを与える</summary>
        static void WarpAndHit(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            entt::entity target,
            const data::FlickerStrikeWeaponData& masterData);

        /// <summary>シーケンスを終了する：無敵化解除、状態リセット</summary>
        static void EndSequence(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerFlickerStrikeComponent& flicker);

        // PickDirectionalTarget()のOverlapSphere結果の一時バッファ。毎回clear()して再利用する
        std::vector<entt::entity> mDirectionalCandidates;
        // UpdateActiveSequence()内: 次のワープ先探索の一時バッファ(ワープのたびにclear()して再利用)
        std::vector<entt::entity> mWarpCandidates;
    };
}
