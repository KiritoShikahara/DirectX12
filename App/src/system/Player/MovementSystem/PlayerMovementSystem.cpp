#include "apppch.h"
#include "PlayerMovementSystem.h"

#include"PlayerMovementComponent.h"
#include"../State/PlayerStateComponent.h"

namespace ecs
{
	void PlayerMovementSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		registry.view<ecs::PlayerMovementComponent, ecs::PlayerStateComponent, ecs::RigidBodyComponent>().each(
			[&](ecs::PlayerMovementComponent& movement, ecs::PlayerStateComponent& state, ecs::RigidBodyComponent& rb)
			{
				if (state.CurrentState != ecs::ePlayerState::Move)
				{
					movement.CurrentSpeed = 0.f;
					movement.MoveInput = { 0.f, 0.f, 0.f };

					rb.MoveVelocity.x = 0.f;
					rb.MoveVelocity.z = 0.f;
					rb.HasMoveRequest = true;
					return;
				}

				const DirectX::XMFLOAT3 horizontalVelocity =
					ComputeHorizontalVelocity(movement, deltaTime);

				rb.MoveVelocity.x += horizontalVelocity.x;
				rb.MoveVelocity.z += horizontalVelocity.z;
				rb.HasMoveRequest = true;
			});
	}

	DirectX::XMFLOAT3 PlayerMovementSystem::ComputeHorizontalVelocity(ecs::PlayerMovementComponent& movement, float deltaTime)
	{
		using namespace DirectX;

		const XMVECTOR input = XMLoadFloat3(&movement.MoveInput);
		const float inputLenSq = XMVectorGetX(XMVector3LengthSq(input));
		const bool hasInput = inputLenSq > 0.f;

		const XMVECTOR dir = hasInput ? XMVector3Normalize(input) : XMVectorZero();

		if (!movement.UseAcceleration)
		{
			movement.CurrentSpeed = hasInput ? movement.MaxSpeed : 0.f;

			XMFLOAT3 immediateResult;
			XMStoreFloat3(&immediateResult, dir * movement.CurrentSpeed);
			return immediateResult;
		}

		const float targetSpeed = hasInput ? movement.MaxSpeed : 0.f;
		const float rate = hasInput ? movement.Acceleration : movement.Deceleration;

		const float diff = targetSpeed - movement.CurrentSpeed;
		const float step = rate * deltaTime;

		if (std::abs(diff) <= step)
		{
			movement.CurrentSpeed = targetSpeed;
		}
		else
		{
			movement.CurrentSpeed += (diff >= 0.f ? step : -step);
		}
		movement.CurrentSpeed = std::max(movement.CurrentSpeed, 0.f);

		const XMVECTOR velocity = dir * movement.CurrentSpeed;

		XMFLOAT3 result;
		XMStoreFloat3(&result, velocity);
		return result;
	}
}