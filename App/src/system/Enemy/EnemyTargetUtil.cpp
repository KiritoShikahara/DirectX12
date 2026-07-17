#include "apppch.h"
#include "EnemyTargetUtil.h"

#include<Tag/EntityTag.h>
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
}
