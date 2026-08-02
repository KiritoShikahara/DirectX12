#include "apppch.h"
#include "TemporaryLifetimeSystem.h"

#include"TemporaryLifetimeComponent.h"

namespace ecs
{
    void TemporaryLifetimeSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        std::vector<entt::entity> expired;

        registry.view<ecs::TemporaryLifetimeComponent>().each(
            [&](entt::entity entity, ecs::TemporaryLifetimeComponent& lifetime)
            {
                lifetime.RemainingTime -= deltaTime;
                if (lifetime.RemainingTime <= 0.0f)
                {
                    expired.push_back(entity);
                }
            });

        for (entt::entity entity : expired)
        {
            registry.destroy(entity);
        }
    }
}
