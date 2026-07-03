#include "apppch.h"
#include "PlayerStateSystem.h"

#include"PlayerStateComponent.h"

namespace ecs
{
	void PlayerStateSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		registry.view<ecs::PlayerStateComponent>().each(
			[&](ecs::PlayerStateComponent& state)
			{
				ResolveRequests(state);
			});
	}

	void PlayerStateSystem::ResolveRequests(ecs::PlayerStateComponent& state)
	{
		if (state.Requests.empty()) return;

		std::sort(state.Requests.begin(), state.Requests.end(),
			[](const ecs::PlayerStateRequest& a, const ecs::PlayerStateRequest& b)
			{
				return a.Priority > b.Priority;
			});

		for (const auto& request : state.Requests)
		{
			if (state.CanTransition(request.State))
			{
				state.CurrentState = request.State;
				break;
			}
		}

		state.Requests.clear();
	}
}