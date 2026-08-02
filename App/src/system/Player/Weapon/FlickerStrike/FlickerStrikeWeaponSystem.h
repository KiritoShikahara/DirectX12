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

    ///<summary>
    ///FlickerStrike型武器の発動ロジック。狙い方向の最初の敵へワープして初撃を与え、パワーチャージを全消費して追加ワープ攻撃を連続で行う
    ///</summary>
    class FlickerStrikeWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///originからdirection方向へ直線範囲内にいる最も近い敵を返す、無ければentt::null
        ///</summary>
        entt::entity PickDirectionalTarget(
            entt::registry& registry,
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& direction,
            float maxRange,
            float width);

        ///<summary>
        ///シーケンスを開始する、パワーチャージを全消費し初撃を与える
        ///</summary>
        static void StartSequence(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            ecs::PlayerFlickerStrikeComponent& flicker,
            entt::entity initialTarget,
            const data::FlickerStrikeWeaponData& masterData);

        ///<summary>
        ///シーケンス中の毎フレーム処理、他スキル入力による中断・ワープ間隔の消化・次の対象探索・終了判定を行う
        ///</summary>
        void UpdateActiveSequence(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            ecs::PlayerFlickerStrikeComponent& flicker,
            const data::FlickerStrikeWeaponData& masterData,
            float deltaTime);

        ///<summary>
        ///対象の近くへ瞬時にワープし、ダメージ・ヒットエフェクトを与える
        ///</summary>
        static void WarpAndHit(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            entt::entity target,
            const data::FlickerStrikeWeaponData& masterData);

        ///<summary>
        ///シーケンスを終了する、無敵化解除と状態リセットを行う
        ///</summary>
        static void EndSequence(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerFlickerStrikeComponent& flicker);

        ///<summary>
        ///ワープ着弾の瞬間に攻撃アニメーションを再生する、非ループ・高速再生
        ///</summary>
        static void PlayAttackAnimation(entt::registry& registry, entt::entity playerEntity);

        ///<summary>
        ///攻撃アニメーションの再生が終わっていれば、次のワープまでの待機中は移動アニメーションへ戻す
        ///</summary>
        static void UpdateTravelAnimation(entt::registry& registry, entt::entity playerEntity);

        ///<summary>
        ///PickDirectionalTargetのOverlapSphere結果の一時バッファ、毎回clearして再利用する
        ///</summary>
        std::vector<entt::entity> mDirectionalCandidates;

        ///<summary>
        ///UpdateActiveSequence内で使う次のワープ先探索の一時バッファ、ワープのたびにclearして再利用する
        ///</summary>
        std::vector<entt::entity> mWarpCandidates;
    };
}
