#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<string>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace graphics { class FbxResource; }

namespace ecs
{
    /// <summary>
    /// ProjectileComponent が SensorEnterEvent で敵と接触したフレームを検知し、
    /// 命中位置を中心とした球形範囲(ExplosionRadius)内の敵全員にダメージを与える。
    /// ノックバック等の物理的な反応は行わない（HP減少のみ）。
    /// 爆発エフェクトを再生した後、弾自体は破棄する。
    /// MaxGeneration/SplitCountが設定された弾(Ricochet等)は、破棄と同時に
    /// SplitCount体の子弾(Generation+1)へ増殖させる(SpawnSplitProjectiles参照)。
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
            float visualRadius,
            bool explosionAtGroundLevel);

        // Update() 内で収集する命中1件分の情報(view走査完了後にまとめてダメージ適用・
        // エフェクト生成するための一時データ。走査中の生成/破棄はイテレータを不正化しうるため避ける)
        struct HitResult
        {
            DirectX::XMFLOAT3 ImpactPos;
            float             ExplosionRadius;
            float             VisualRadius;
            float             Damage;
            std::string       ExplosionEffectPath;
            bool              ExplosionAtGroundLevel;
        };

        // 命中した弾が増殖対象だった場合、view走査完了後にまとめて子弾を生成するための一時データ
        struct SplitRequest
        {
            DirectX::XMFLOAT3   ImpactPos;
            entt::entity        ExcludedEnemy; // 命中した敵自身(次の対象探索から除外する)
            int                 NextGeneration;
            int                 SplitCount;
            int                 MaxGeneration;
            float               SplitSearchRadius;
            // 子弾へそのまま引き継ぐ親弾のパラメータ(Damage/Speed等は世代が変わっても減衰させない)
            float               Speed;
            float               Damage;
            float               ExplosionRadius;
            float               VisualRadius;
            std::string         ExplosionEffectPath;
            float               LifeTime;
            entt::entity        Owner;
            graphics::FbxResource* VisualMeshResource;
            float               VisualMeshScale;
            DirectX::XMFLOAT4   VisualMeshColor;
        };

        /// <summary>命中位置のSplitSearchRadius内から(命中した敵を除いて)ランダムに
        /// 最大SplitCount体の敵を選び、それぞれへ向かう子弾(Generation+1)を生成する</summary>
        void SpawnSplitProjectiles(entt::registry& registry, const SplitRequest& request);

        // Update()の一時バッファ。毎回clear()して再利用する(毎フレームのvector生成禁止のため)
        std::vector<entt::entity>  mHitProjectiles;
        std::vector<HitResult>     mHitResults;
        std::vector<SplitRequest>  mSplitRequests;
        // ApplyExplosionDamage()のOverlapSphere結果の一時バッファ(命中ごとにclear()して再利用)
        std::vector<entt::entity> mOverlapped;
        // SpawnSplitProjectiles()のOverlapSphere結果/敵フィルタ結果の一時バッファ
        std::vector<entt::entity> mSplitFound;
        std::vector<entt::entity> mSplitEnemies;
    };
}
