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
		if (state.Requests.empty())
		{
			state.CurrentState = ePlayerState::Idle;
			return;
		};

		// リクエストの優先度に基づいてソート
		std::sort(state.Requests.begin(), state.Requests.end(),
			[](const ecs::PlayerStateRequest& a, const ecs::PlayerStateRequest& b)
			{
				return a.Priority > b.Priority;
			});

		// 優先度の高いリクエストから順に処理
		for (const auto& request : state.Requests)
		{
			if (state.CurrentState == request.State)
			{
				break;
			}

			if (state.CanTransition(request.State))
			{
				state.CurrentState = request.State;
				break;
			}
		}

		state.Requests.clear();
	}
}