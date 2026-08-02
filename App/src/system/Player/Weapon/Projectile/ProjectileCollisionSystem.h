#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<string>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace graphics { class FbxResource; }

namespace ecs
{
    ///<summary>
    ///ProjectileComponentが敵と接触したフレームを検知し、命中位置中心の球形範囲内の敵全員にダメージを与える。増殖弾は子弾生成も行う
    ///</summary>
    class ProjectileCollisionSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///命中位置に爆発ダメージを適用する、敵タグ以外は無視する
        ///</summary>
        void ApplyExplosionDamage(
            entt::registry& registry,
            const DirectX::XMFLOAT3& center,
            float radius,
            float damage);

        ///<summary>
        ///着弾エフェクトを一度だけ再生する一時エンティティを生成する
        ///</summary>
        static void SpawnExplosionEffect(
            entt::registry& registry,
            const DirectX::XMFLOAT3& position,
            const std::string& effectPath,
            float hitRadius,
            float visualRadius,
            bool explosionAtGroundLevel);

        ///<summary>
        ///Update内で収集する命中1件分の情報、view走査完了後にまとめてダメージ適用・エフェクト生成するための一時データ
        ///</summary>
        struct HitResult
        {
            DirectX::XMFLOAT3 ImpactPos;
            float             ExplosionRadius;
            float             VisualRadius;
            float             Damage;
            std::string       ExplosionEffectPath;
            bool              ExplosionAtGroundLevel;
        };

        ///<summary>
        ///命中した弾が増殖対象だった場合、view走査完了後にまとめて子弾を生成するための一時データ
        ///</summary>
        struct SplitRequest
        {
            DirectX::XMFLOAT3   ImpactPos;
            entt::entity        ExcludedEnemy;
            int                 NextGeneration;
            int                 SplitCount;
            int                 MaxGeneration;
            float               SplitSearchRadius;
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

        ///<summary>
        ///命中位置のSplitSearchRadius内から命中した敵を除いてランダムに最大SplitCount体を選び子弾を生成する
        ///</summary>
        void SpawnSplitProjectiles(entt::registry& registry, const SplitRequest& request);

        ///<summary>
        ///Updateの一時バッファ、毎回clearして再利用する
        ///</summary>
        std::vector<entt::entity>  mHitProjectiles;
        std::vector<HitResult>     mHitResults;
        std::vector<SplitRequest>  mSplitRequests;

        ///<summary>
        ///ApplyExplosionDamageのOverlapSphere結果の一時バッファ、命中ごとにclearして再利用する
        ///</summary>
        std::vector<entt::entity> mOverlapped;

        ///<summary>
        ///SpawnSplitProjectilesのOverlapSphere結果/敵フィルタ結果の一時バッファ
        ///</summary>
        std::vector<entt::entity> mSplitFound;
        std::vector<entt::entity> mSplitEnemies;
    };
}
