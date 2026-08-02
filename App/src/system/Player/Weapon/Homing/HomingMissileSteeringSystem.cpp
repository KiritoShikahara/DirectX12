#include "apppch.h"
#include "HomingMissileSteeringSystem.h"

#include<system/Player/Weapon/Projectile/ProjectileComponent.h>
#include<system/Player/PlayerActionLock.h>
#include<system/Enemy/EnemyTargetUtil.h>
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
                    projectile.Target = ecs::targetutil::FindNearestInRadius(
                        registry, transform.GetPosition(), projectile.HomingSearchRadius);
                }

                if (!registry.valid(projectile.Target)) return; // 対象なしなら直進を維持

                const auto* targetTransform = registry.try_get<ecs::Transform>(projectile.Target);
                if (targetTransform == nullptr) return;

                SteerTowards(projectile.Direction, transform.GetPosition(), targetTransform->GetPosition(), projectile.TurnSpeed, deltaTime);
            });
    }

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
