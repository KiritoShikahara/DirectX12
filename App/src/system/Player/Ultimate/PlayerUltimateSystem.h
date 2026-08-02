#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct PlayerUltimateComponent; }
namespace ecs { struct PlayerStatusComponent; }
namespace data { struct UltimateData; }

namespace ecs
{
    ///<summary>
    ///プレイヤーの必殺技を処理するシステム。撃破数閾値でゲージ満タンからオーラ付与、Ultimateアクションで発動し、上昇→ビーム→メインエフェクトの順に再生してから座標を戻し全体ダメージを与える。InGame状態の時だけ動作する
    ///</summary>
    class PlayerUltimateSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///ゲージが満タンになった瞬間、オーラのループ・複数対応エフェクトをプレイヤーへ付与する
        ///</summary>
        static void StartAura(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerUltimateComponent& ultimate,
            const data::UltimateData& masterData);

        ///<summary>
        ///必殺技を発動する。オーラ解除、無敵化、カメラを即座に固定、上昇開始
        ///</summary>
        static void Activate(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerUltimateComponent& ultimate,
            ecs::PlayerStatusComponent& status,
            const data::UltimateData& masterData);

        ///<summary>
        ///発動中の毎フレーム処理。Ascending/PlayingBeam/PlayingMainフェーズの遷移判定
        ///</summary>
        static void UpdateActive(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerUltimateComponent& ultimate,
            ecs::PlayerStatusComponent& status,
            const data::UltimateData& masterData,
            float rawDeltaTime);

        ///<summary>
        ///プレイヤー背後・高い位置から見下ろす構図になるようカメラへ位置リクエストを発行する。実際のTransform書き込みはCameraPlayerFollowSystemが行う
        ///</summary>
        static void UpdateCamera(
            entt::registry& registry,
            entt::entity playerEntity,
            const ecs::PlayerUltimateComponent& ultimate,
            const data::UltimateData& masterData);

        ///<summary>
        ///メイン終了後。プレイヤー座標・無敵状態を戻し、その場で全体ダメージを与える
        ///</summary>
        static void FinishAndExplode(
            entt::registry& registry,
            entt::entity playerEntity,
            ecs::PlayerUltimateComponent& ultimate,
            ecs::PlayerStatusComponent& status,
            const data::UltimateData& masterData);
    };
}
