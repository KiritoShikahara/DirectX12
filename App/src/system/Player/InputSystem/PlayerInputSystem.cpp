#include "apppch.h"
#include "PlayerInputSystem.h"

#include<Tag/EntityTag.h>
#include"../MovementSystem/PlayerMovementComponent.h"
#include"../State/PlayerStateComponent.h"
#include"../AimSysten/PlayerAimComponent.h"
#include<system/Player/PlayerActionLock.h>
#include<Scene/Game/State/GameState.h>

namespace ecs
{
	void PlayerInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		const auto& moveAxis = INPUT_MANAGER.GetMoveAxis();
		const bool hasMoveInput = (moveAxis.x != 0.f || moveAxis.y != 0.f);
		const bool wantsToFire = INPUT_MANAGER.IsActionPressed("Attack");
		const bool wantsToFireSecondary = INPUT_MANAGER.IsActionPressed("Attack2");
		const bool wantsToFireTertiary = INPUT_MANAGER.IsActionPressed("FlickerStrike");
		// 必殺技/Flicker Strikeの演出中、および設定メニュー表示中はプレイヤー操作を一切受け付けない
		const bool inputBlocked = ecs::IsPlayerActionLocked(registry) || ecs::IsOptionsMenuOpen(registry);

		registry.view<ecs::PlayerMovementComponent, ecs::PlayerStateComponent, ecs::PlayerAimComponent, ecs::PlayerTag>().each(
			[&](ecs::PlayerMovementComponent& movement, ecs::PlayerStateComponent& state, ecs::PlayerAimComponent& aim)
			{
				if (inputBlocked)
				{
					movement.MoveInput = { 0.f, 0.f, 0.f };
					aim.WantsToFire = false;
					aim.WantsToFireSecondary = false;
					aim.WantsToFireTertiary = false;
					return;
				}

				if (hasMoveInput)
				{
					movement.MoveInput = { moveAxis.x, 0.f, moveAxis.y };
					state.AddRequest(ePlayerState::Move, 1);
				}

				aim.WantsToFire = wantsToFire;
				aim.WantsToFireSecondary = wantsToFireSecondary;
				aim.WantsToFireTertiary = wantsToFireTertiary;
			});
	}
}

