#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct WeaponComponent; }
namespace data { struct ChainLightningWeaponData; }

namespace ecs
{
    ///<summary>
    ///Chain Lightning型武器を処理するシステム。命中した敵から近くの敵へ自動で跳ね移る完全自動の雷撃武器
    ///</summary>
    class ChainLightningWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///最初の対象へ命中させ、JumpRadius内の未命中の敵へ最大MaxJumps回まで跳ね移らせる
        ///</summary>
        void Zap(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const data::ChainLightningWeaponData& masterData,
            entt::entity initialTarget);

        ///<summary>
        ///命中位置にワンショットのヒットエフェクトを再生する
        ///</summary>
        static void SpawnHitEffect(
            entt::registry& registry,
            const DirectX::XMFLOAT3& position,
            const data::ChainLightningWeaponData& masterData);

        ///<summary>
        ///初撃対象探索用の一時バッファ、毎回clearして再利用する
        ///</summary>
        std::vector<entt::entity> mFound;

        ///<summary>
        ///Zap内で跳躍済みの対象一覧、Zap呼び出しごとにクリアする
        ///</summary>
        std::vector<entt::entity> mVisited;

        ///<summary>
        ///Zap内で跳躍先探索の候補一時バッファ、跳躍のたびにclearして再利用する
        ///</summary>
        std::vector<entt::entity> mCandidates;
    };
}
