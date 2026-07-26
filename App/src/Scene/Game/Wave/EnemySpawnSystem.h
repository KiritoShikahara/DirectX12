#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct EnemyWaveModifier; }

namespace ecs
{
    /// <summary>
    /// ウェーブサバイバルのコアループを回すシステム。
    /// 経過時間の管理・敵の継続スポーン・難易度上昇・ボース出現・クリア判定を行う。
    /// InGame状態のときのみ動作する。
    /// </summary>
    class EnemySpawnSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>
        /// 画面外(画面に映っている範囲の半径 + マージン)のリング上にランダムなスポーン位置を求める
        /// </summary>
        static DirectX::XMFLOAT3 ComputeSpawnPosition(
            entt::registry& registry,
            const DirectX::XMFLOAT3& playerPos,
            float marginMin, float marginMax);

        /// <summary>
        /// 現在のカメラ設定で、プレイヤーの足元平面上に画面(四隅)が投影される範囲の半径を求める。
        /// カメラが無い場合はフォールバック値を返す。
        /// </summary>
        static float ComputeVisibleRadius(entt::registry& registry, const DirectX::XMFLOAT3& playerPos);

        /// <summary>経過時間から現在の敵ステータス成長倍率を求める(stepInterval秒ごとにgrowthPerStep分、階段状に成長)</summary>
        static ecs::EnemyWaveModifier ComputeWaveModifier(float elapsedTime, float stepInterval, float growthPerStep);
    };
}
