#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<string>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    /// <summary>
    /// ProjectileComponent が SensorEnterEvent で敵と接触したフレームを検知し、
    /// 命中位置を中心とした球形範囲(ExplosionRadius)内の敵全員にダメージを与える。
    /// ノックバック等の物理的な反応は行わない（HP減少のみ）。
    /// 爆発エフェクトを再生した後、弾自体は破棄する。
    /// </summary>
    class ProjectileCollisionSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>命中位置に爆発ダメージを適用する（敵タグ以外は無視する）</summary>
        void ApplyExplosionDamage(
            entt::registry& registry,
            const DirectX::XMFLOAT3& center,
            float radius,
            float damage);

        /// <summary>着弾エフェクトを一度だけ再生する一時エンティティを生成する</summary>
        static void SpawnExplosionEffect(
            entt::registry& registry,
            const DirectX::XMFLOAT3& position,
            const std::string& effectPath,
            float hitRadius,
            float visualRadius);

        // Update() 内で収集する命中1件分の情報(view走査完了後にまとめてダメージ適用・
        // エフェクト生成するための一時データ。走査中の生成/破棄はイテレータを不正化しうるため避ける)
        struct HitResult
        {
            DirectX::XMFLOAT3 ImpactPos;
            float             ExplosionRadius;
            float             VisualRadius;
            float             Damage;
            std::string       ExplosionEffectPath;
        };

        // Update()の一時バッファ。毎回clear()して再利用する(毎フレームのvector生成禁止のため)
        std::vector<entt::entity> mHitProjectiles;
        std::vector<HitResult>    mHitResults;
        // ApplyExplosionDamage()のOverlapSphere結果の一時バッファ(命中ごとにclear()して再利用)
        std::vector<entt::entity> mOverlapped;
    };
}
