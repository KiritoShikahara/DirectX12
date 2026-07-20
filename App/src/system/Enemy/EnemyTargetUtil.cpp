#include "apppch.h"
#include "EnemyTargetUtil.h"

#include<Tag/EntityTag.h>
#include<system/Physics/System/PhysicsSystem.h>
#include<algorithm>

namespace ecs::targetutil
{
    entt::entity FindNearestExcluding(
        entt::registry& registry,
        const std::vector<entt::entity>& candidates,
        const DirectX::XMFLOAT3& position,
        const std::vector<entt::entity>& excluded)
    {
        entt::entity nearest = entt::null;
        float nearestDistSq = 0.0f;
        bool found = false;

        for (entt::entity entity : candidates)
        {
            if (!registry.all_of<ecs::EnemyTag>(entity)) continue;
            if (std::find(excluded.begin(), excluded.end(), entity) != excluded.end()) continue;

            const auto* transform = registry.try_get<ecs::Transform>(entity);
            if (transform == nullptr) continue;

            const DirectX::XMFLOAT3& pos = transform->GetPosition();
            const float dx = pos.x - position.x;
            const float dz = pos.z - position.z;
            const float distSq = dx * dx + dz * dz;

            if (!found || distSq < nearestDistSq)
            {
                nearestDistSq = distSq;
                nearest = entity;
                found = true;
            }
        }

        return nearest;
    }

    entt::entity FindNearestInRadius(
        entt::registry& registry,
        const DirectX::XMFLOAT3& position,
        float radius,
        const std::vector<entt::entity>& excluded)
    {
        // 呼び出しごとのヒープ確保を避けるため、関数内staticとして使い回す
        // （EnemySpawnSystemの乱数エンジンと同じく、プロセス全体で1つを使い回す流儀）。
        // 現状の唯一の呼び出し元(HomingMissileSteeringSystem::Update)は
        // シングルスレッドのシステム更新のため、再入や並行呼び出しは発生しない。
        static std::vector<entt::entity> candidates;
        candidates.clear();
        ::sys::PhysicsSystem::OverlapSphere(registry, position, radius, candidates);

        return FindNearestExcluding(registry, candidates, position, excluded);
    }
}
