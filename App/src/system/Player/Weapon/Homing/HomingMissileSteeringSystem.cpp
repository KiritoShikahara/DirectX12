#include "apppch.h"
#include "HomingMissileSteeringSystem.h"

#include<system/Player/Weapon/Projectile/ProjectileComponent.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<Tag/EntityTag.h>

#include<cmath>

namespace ecs
{
    void HomingMissileSteeringSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // 必殺技演出中は既存の追尾弾の旋回も一時停止させる
        if (ecs::IsPlayerUltimateActive(registry)) return;

        registry.view<ecs::ProjectileComponent, ecs::Transform>().each(
            [&](ecs::ProjectileComponent& projectile, ecs::Transform& transform)
            {
                if (!projectile.IsHoming) return;

                const bool targetAlive =
                    registry.valid(projectile.Target) &&
                    registry.all_of<ecs::EnemyTag>(projectile.Target);

                if (!targetAlive)
                {
                    projectile.Target = FindNearestEnemy(registry, transform.GetPosition(), projectile.HomingSearchRadius);
                }

                if (!registry.valid(projectile.Target)) return; // 対象なしなら直進を維持

                const auto* targetTransform = registry.try_get<ecs::Transform>(projectile.Target);
                if (targetTransform == nullptr) return;

                SteerTowards(projectile.Direction, transform.GetPosition(), targetTransform->GetPosition(), projectile.TurnSpeed, deltaTime);
            });
    }

    /// <summary>指定範囲内で最も近い敵エンティティを探す（見つからなければentt::null）</summary>
    entt::entity HomingMissileSteeringSystem::FindNearestEnemy(
        entt::registry& registry,
        const DirectX::XMFLOAT3& position,
        float searchRadius)
    {
        entt::entity nearest = entt::null;
        float nearestDistSq = searchRadius * searchRadius;

        registry.view<ecs::EnemyTag, ecs::Transform>().each(
            [&](entt::entity entity, const ecs::Transform& enemyTransform)
            {
                const DirectX::XMFLOAT3& enemyPos = enemyTransform.GetPosition();
                const float dx = enemyPos.x - position.x;
                const float dz = enemyPos.z - position.z;
                const float distSq = dx * dx + dz * dz;

                if (distSq < nearestDistSq)
                {
                    nearestDistSq = distSq;
                    nearest = entity;
                }
            });

        return nearest;
    }

    /// <summary>DirectionをtargetPos方向へTurnSpeed*deltaTimeの範囲内で回転させる（XZ平面のみ、Yは常に0）</summary>
    void HomingMissileSteeringSystem::SteerTowards(
        DirectX::XMFLOAT3& direction,
        const DirectX::XMFLOAT3& fromPos,
        const DirectX::XMFLOAT3& targetPos,
        float turnSpeedDegrees,
        float deltaTime)
    {
        using namespace DirectX;

        const float dx = targetPos.x - fromPos.x;
        const float dz = targetPos.z - fromPos.z;
        if (dx * dx + dz * dz < 0.0001f) return; // 到達済みに近い場合は向きを維持

        const float currentAngle = std::atan2(direction.z, direction.x);
        const float desiredAngle = std::atan2(dz, dx);

        const float maxTurn = XMConvertToRadians(turnSpeedDegrees) * deltaTime;
        const float angleDiff = XMScalarModAngle(desiredAngle - currentAngle);
        const float clampedDiff = std::clamp(angleDiff, -maxTurn, maxTurn);

        const float newAngle = currentAngle + clampedDiff;
        direction = { std::cos(newAngle), 0.0f, std::sin(newAngle) };
    }
}
