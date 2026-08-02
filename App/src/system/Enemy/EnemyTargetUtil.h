#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs::targetutil
{
    ///<summary>
    ///candidatesのうちEnemyTagを持ち、positionに最も近くexcludedに含まれない敵を返す。見つからなければentt::null
    ///</summary>
    entt::entity FindNearestExcluding(
        entt::registry& registry,
        const std::vector<entt::entity>& candidates,
        const DirectX::XMFLOAT3& position,
        const std::vector<entt::entity>& excluded);

    ///<summary>
    ///positionを中心とした半径radius内で、excludedに含まれない最も近い敵を返す。見つからなければentt::null
    ///</summary>
    entt::entity FindNearestInRadius(
        entt::registry& registry,
        const DirectX::XMFLOAT3& position,
        float radius,
        const std::vector<entt::entity>& excluded = {});
}
