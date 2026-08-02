#include "apppch.h"
#include "EnemyKnockbackSystem.h"

#include"EnemyKnockbackComponent.h"

namespace ecs
{
    void EnemyKnockbackSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        std::vector<entt::entity> expired;

        registry.view<EnemyKnockbackComponent, RigidBodyComponent>().each(
            [&](entt::entity entity, EnemyKnockbackComponent& knockback, RigidBodyComponent& rigidBody)
            {
                knockback.RemainingTime -= deltaTime;
                if (knockback.RemainingTime <= 0.0f)
                {
                    expired.push_back(entity);
                    return;
                }

                rigidBody.MoveVelocity = knockback.Velocity;
                rigidBody.HasMoveRequest = true;
            });

        // view走査完了後にまとめて取り外す。走査中の除去はイテレータを不正化しうるため避ける
        for (entt::entity entity : expired)
        {
            registry.remove<EnemyKnockbackComponent>(entity);
        }
    }
}
