#include "apppch.h"
#include "PlayerInputSystem.h"

#include<Tag/EntityTag.h>
#include"../MovementSystem/PlayerMovementComponent.h"
#include"../State/PlayerStateComponent.h"
#include"../AimSysten/PlayerAimComponent.h"
#include"../Ultimate/PlayerUltimateComponent.h"

namespace ecs
{
	void PlayerInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		const auto& moveAxis = INPUT_MANAGER.GetMoveAxis();
		const bool hasMoveInput = (moveAxis.x != 0.f || moveAxis.y != 0.f);
		const bool wantsToFire = INPUT_MANAGER.IsActionPressed("Attack");
		const bool wantsToFireSecondary = INPUT_MANAGER.IsActionPressed("Attack2");

		registry.view<ecs::PlayerMovementComponent, ecs::PlayerStateComponent, ecs::PlayerAimComponent, ecs::PlayerUltimateComponent, ecs::PlayerTag>().each(
			[&](ecs::PlayerMovementComponent& movement, ecs::PlayerStateComponent& state, ecs::PlayerAimComponent& aim, const ecs::PlayerUltimateComponent& ultimate)
			{
				// 必殺技演出中はプレイヤー操作を一切受け付けない（移動・攻撃とも入力を無視する）
				if (ultimate.IsActive)
				{
					movement.MoveInput = { 0.f, 0.f, 0.f };
					aim.WantsToFire = false;
					aim.WantsToFireSecondary = false;
					return;
				}

				if (hasMoveInput)
				{
					movement.MoveInput = { moveAxis.x, 0.f, moveAxis.y };
					state.AddRequest(ePlayerState::Move, 1);
				}

				aim.WantsToFire = wantsToFire;
				aim.WantsToFireSecondary = wantsToFireSecondary;
			});
	}
}

