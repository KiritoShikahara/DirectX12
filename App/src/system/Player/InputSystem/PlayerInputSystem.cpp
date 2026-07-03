#include "apppch.h"
#include "PlayerInputSystem.h"

#include<Tag/EntityTag.h>
#include"../MovementSystem/PlayerMovementComponent.h"
#include"../State/PlayerStateComponent.h"

namespace ecs
{
	void PlayerInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		const auto& moveAxis = INPUT_MANAGER.GetMoveAxis();
		const bool hasInput = (moveAxis.x != 0.f || moveAxis.y != 0.f);

		if (hasInput == false)
		{
			return;
		}

		registry.view<ecs::PlayerMovementComponent, ecs::PlayerStateComponent, ecs::PlayerTag>().each(
			[&](ecs::PlayerMovementComponent& movement, ecs::PlayerStateComponent& state)
			{
				movement.MoveInput = { moveAxis.x, 0.f, moveAxis.y };
				state.AddRequest(ePlayerState::Move, 1);
			});
	}
}

