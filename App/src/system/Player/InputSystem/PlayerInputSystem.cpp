#include "apppch.h"
#include "PlayerInputSystem.h"

#include<Tag/EntityTag.h>
#include"../MovementSystem/PlayerMovementComponent.h"

namespace ecs
{
	void PlayerInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		const auto& moveAxis = INPUT_MANAGER.GetMoveAxis();

		registry.view<ecs::PlayerMovementComponent, ecs::PlayerTag>().each(
			[&](ecs::PlayerMovementComponent& movement)
			{
				movement.MoveInput = { moveAxis.x, 0.f, moveAxis.y };
			});
	}
}

