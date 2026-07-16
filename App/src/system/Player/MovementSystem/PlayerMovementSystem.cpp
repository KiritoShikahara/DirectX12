#include "apppch.h"
#include "PlayerMovementSystem.h"

#include"PlayerMovementComponent.h"
#include"../State/PlayerStateComponent.h"
#include"../Status/PlayerStatusComponent.h"
#include<system/MoveDirection/MoveDirectionComponent.h>

namespace ecs
{
	void PlayerMovementSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		registry.view<ecs::PlayerMovementComponent, ecs::PlayerStateComponent, ecs::RigidBodyComponent, ecs::MoveDirectionComponent>().each(
			[&](entt::entity entity, ecs::PlayerMovementComponent& movement, ecs::PlayerStateComponent& state, ecs::RigidBodyComponent& rb, ecs::MoveDirectionComponent& moveDir)
			{
				if (state.CurrentState != ecs::ePlayerState::Move)
				{
					movement.CurrentSpeed = 0.f;
					movement.MoveInput = { 0.f, 0.f, 0.f };

					rb.MoveVelocity.x = 0.f;
					rb.MoveVelocity.z = 0.f;
					rb.HasMoveRequest = true;

					moveDir.IsMoving = false;
					return;
				}

				// PlayerStatusComponent::Current.MoveSpeed(パークで強化される値)は
				// MaxSpeedとは単位が異なるため、Base比の比率として掛け合わせる
				float speedMultiplier = 1.0f;
				if (const auto* status = registry.try_get<ecs::PlayerStatusComponent>(entity);
					status != nullptr && status->Base.MoveSpeed > 0.0f)
				{
					speedMultiplier = status->Current.MoveSpeed / status->Base.MoveSpeed;
				}

				const DirectX::XMFLOAT3 horizontalVelocity =
					ComputeHorizontalVelocity(movement, deltaTime, speedMultiplier);

				rb.MoveVelocity.x = horizontalVelocity.x;
				rb.MoveVelocity.z = horizontalVelocity.z;
				rb.HasMoveRequest = true;

				using namespace DirectX;
				const XMVECTOR vel = XMVectorSet(horizontalVelocity.x, 0.f, horizontalVelocity.z, 0.f);
				const bool isMoving = XMVectorGetX(XMVector3LengthSq(vel)) > 0.0001f;

				moveDir.IsMoving = isMoving;
				if (isMoving)
				{
					XMStoreFloat3(&moveDir.Direction, XMVector3Normalize(vel));
				}
			});
	}

	DirectX::XMFLOAT3 PlayerMovementSystem::ComputeHorizontalVelocity(ecs::PlayerMovementComponent& movement, float deltaTime, float speedMultiplier)
	{
		using namespace DirectX;

		const XMVECTOR input = XMLoadFloat3(&movement.MoveInput);
		const float inputLen = XMVectorGetX(XMVector3Length(input));
		const bool hasInput = inputLen > 0.f;

		const XMVECTOR dir = hasInput ? XMVector3Normalize(input) : XMVectorZero();
		const float inputScale = std::min(inputLen, 1.0f);
		const float maxSpeed = movement.MaxSpeed * speedMultiplier;

		if (!movement.UseAcceleration)
		{
			movement.CurrentSpeed = hasInput ? maxSpeed * inputScale : 0.f;

			XMFLOAT3 immediateResult;
			XMStoreFloat3(&immediateResult, dir * movement.CurrentSpeed);
			return immediateResult;
		}

		const float targetSpeed = hasInput ? maxSpeed * inputScale : 0.f;
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