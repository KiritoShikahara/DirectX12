#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct PlayerUltimateComponent; }
namespace ecs { struct PlayerStatusComponent; }
namespace data { struct UltimateData; }

namespace ecs
{
    /// <summary>
    /// プレイヤーの必殺技(Ultimate)を処理するシステム。
    /// 撃破数の閾値到達でゲージ満タン→オーラ付与、"Ultimate"アクション(Qキー/PadR1)入力で発動。
    ///
    /// 発動後は時間ではなく状態で遷移する: (1)カメラを即座にプレイヤー背後・高い位置(見下ろす
    /// 構図)へ固定 → (2)RiseHeightに達するまで上昇 → (3)ビーム(pre)エフェクトを再生しその終了を
    /// 待つ → (4)同じ位置でメイン(main)エフェクトを再生しその終了を待つ → (5)プレイヤー座標・
    /// カメラを瞬時に発動前へ戻し(テレポート)、その場で全体ダメージ。この間プレイヤーは
    /// 無敵化され、PlayerInputSystemが操作を無効化する。InGame状態のときのみ動作する。
    /// </summary>
    class PlayerUltimateSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>ゲージが満タンになった瞬間、オーラ(ループ、複数対応)エフェクトをプレイヤーへ付与する</summary>
        static void StartAura(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerUltimateComponent& ultimate,
            const data::UltimateData& masterData);

        /// <summary>必殺技を発動する：オーラ解除、無敵化、カメラを即座に固定、上昇開始</summary>
        static void Activate(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerUltimateComponent& ultimate,
            ecs::PlayerStatusComponent& status,
            const data::UltimateData& masterData);

        /// <summary>発動中(IsActive)の毎フレーム処理：Ascending/PlayingBeam/PlayingMainフェーズの遷移判定</summary>
        static void UpdateActive(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerUltimateComponent& ultimate,
            ecs::PlayerStatusComponent& status,
            const data::UltimateData& masterData,
            float rawDeltaTime);

        /// <summary>プレイヤー背後・高い位置から見下ろす構図になるようカメラへ位置リクエストを発行する
        /// (実際のTransform書き込みはCameraPlayerFollowSystemが行う)</summary>
        static void UpdateCamera(
            entt::registry& registry,
            entt::entity playerEntity,
            const ecs::PlayerUltimateComponent& ultimate,
            const data::UltimateData& masterData);

        /// <summary>メイン(main)終了後：プレイヤー座標・無敵状態を戻し、その場で全体ダメージを与える</summary>
        static void FinishAndExplode(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerUltimateComponent& ultimate,
            ecs::PlayerStatusComponent& status,
            const data::UltimateData& masterData);
    };
}
