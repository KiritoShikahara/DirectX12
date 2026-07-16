#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<string>
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
        static void ApplyExplosionDamage(
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
    };
}
