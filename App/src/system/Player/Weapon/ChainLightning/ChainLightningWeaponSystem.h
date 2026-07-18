#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct WeaponComponent; }
namespace data { struct ChainLightningWeaponData; }

namespace ecs
{
    /// <summary>
    /// Chain Lightning型武器（命中した敵から近くの敵へ自動で跳ね移る雷撃）を処理するシステム。
    /// 狙い・移動を必要としない完全自動の武器。移動する実体を持たず、命中は瞬時に解決される。
    /// InGame状態のときのみ動作する。
    /// </summary>
    class ChainLightningWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>最初の対象へ命中させ、JumpRadius内の未命中の敵へ最大MaxJumps回まで跳ね移らせる</summary>
        void Zap(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const data::ChainLightningWeaponData& masterData,
            entt::entity initialTarget);

        /// <summary>命中位置にワンショットのヒットエフェクトを再生する</summary>
        static void SpawnHitEffect(
            entt::registry& registry,
            const DirectX::XMFLOAT3& position,
            const data::ChainLightningWeaponData& masterData);

        // 初撃対象探索用の一時バッファ(Update())。毎回clear()して再利用する
        std::vector<entt::entity> mFound;
        // Zap()内: 跳躍済みの対象一覧(Zap呼び出しごとにクリア)
        std::vector<entt::entity> mVisited;
        // Zap()内: 跳躍先探索の候補一時バッファ(跳躍のたびにclear()して再利用)
        std::vector<entt::entity> mCandidates;
    };
}
