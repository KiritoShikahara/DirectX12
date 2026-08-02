#include "apppch.h"
#include "EnemySlowStatusSystem.h"
#include "EnemySlowStatusComponent.h"

namespace ecs
{
	void EnemySlowStatusSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		mExpired.clear();

		registry.view<EnemySlowStatusComponent>().each(
			[&](entt::entity entity, EnemySlowStatusComponent& slow)
			{
				slow.RemainingDuration -= deltaTime;
				if (slow.RemainingDuration <= 0.0f)
				{
					mExpired.push_back(entity);
				}
			});

		for (entt::entity entity : mExpired)
		{
			registry.remove<EnemySlowStatusComponent>(entity);
		}
	}
}
