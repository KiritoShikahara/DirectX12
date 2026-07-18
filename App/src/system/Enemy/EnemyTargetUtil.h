#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs::targetutil
{
    /// <summary>
    /// candidatesのうち、EnemyTagを持ち、positionに最も近く、excludedに含まれない敵を返す
    /// (見つからなければentt::null)。Chain LightningのJumpRadius内探索・Flicker Strikeの
    /// ワープ先探索など、「近くの未処理の敵を1体選ぶ」系のロジックが複数の武器で必要になったため
    /// 共通化した(元々はChainLightningWeaponSystemの private static メンバーだった)。
    /// </summary>
    entt::entity FindNearestExcluding(
        entt::registry& registry,
        const std::vector<entt::entity>& candidates,
        const DirectX::XMFLOAT3& position,
        const std::vector<entt::entity>& excluded);

    /// <summary>
    /// positionを中心とした半径radius内で、excludedに含まれない最も近い敵を返す
    /// (見つからなければentt::null)。PhysicsSystem::OverlapSphereでのブロードフェーズ絞り込みと
    /// FindNearestExcluding()をまとめた便利関数。Homing Missile/Void Beam/Bone Spearが
    /// 「発射方向を決めるため近くの敵を1体探す」目的で個別に(一部はOverlapSphereすら経由せず
    /// 敵全体をフルスキャンする形で)重複実装していたため共通化した。
    /// </summary>
    entt::entity FindNearestInRadius(
        entt::registry& registry,
        const DirectX::XMFLOAT3& position,
        float radius,
        const std::vector<entt::entity>& excluded = {});
}
