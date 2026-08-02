#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct EnemyWaveModifier; }

namespace ecs
{
    ///<summary>
    ///ウェーブサバイバルのコアループを回すシステム。経過時間管理・敵スポーン・難易度上昇・ボス出現・クリア判定をInGame状態の時だけ行う
    ///</summary>
    class EnemySpawnSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///画面外のリング上にランダムなスポーン位置を求める。範囲は画面に映っている半径にマージンを足したもの
        ///</summary>
        static DirectX::XMFLOAT3 ComputeSpawnPosition(
            entt::registry& registry,
            const DirectX::XMFLOAT3& playerPos,
            float marginMin, float marginMax);

        ///<summary>
        ///現在のカメラ設定でプレイヤーの足元平面上に画面四隅が投影される範囲の半径を求める。カメラが無い場合はフォールバック値を返す
        ///</summary>
        static float ComputeVisibleRadius(entt::registry& registry, const DirectX::XMFLOAT3& playerPos);

        ///<summary>
        ///経過時間から現在の敵ステータス成長倍率を求める。stepInterval秒ごとにgrowthPerStep分だけ階段状に成長する
        ///</summary>
        static ecs::EnemyWaveModifier ComputeWaveModifier(float elapsedTime, float stepInterval, float growthPerStep);
    };
}
