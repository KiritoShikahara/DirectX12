#include "apppch.h"
#include "RotateToMoveSystem.h"

#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>
#include"RotateToMoveComponent.h"
#include"../MoveDirection/MoveDirectionComponent.h"

void ecs::RotateToMoveSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{
	using namespace DirectX;

	registry.view<ecs::RotateToMoveComponent, ecs::Transform, ecs::MoveDirectionComponent>().each(
		[&](ecs::RotateToMoveComponent& rotate, ecs::Transform& transform, ecs::MoveDirectionComponent& moveDir)
		{
			if (!moveDir.IsMoving) return;

			const XMVECTOR dir = XMLoadFloat3(&moveDir.Direction);

			const float yaw = std::atan2(XMVectorGetX(dir), XMVectorGetZ(dir));
			XMVECTOR targetRotation = XMQuaternionRotationRollPitchYaw(0.f, yaw, 0.f);

			if (rotate.InstantRotate)
			{
				transform.SetRotation(targetRotation);
				return;
			}

			const XMVECTOR currentRotation = XMLoadFloat4(&transform.GetRotation());
			const float maxRadians = XMConvertToRadians(rotate.RotationSpeedDeg) * deltaTime;

			float dot = XMVectorGetX(XMQuaternionDot(currentRotation, targetRotation));
			if (dot < 0.0f)
			{
				targetRotation = XMVectorNegate(targetRotation);
				dot = -dot;
			}
			dot = std::clamp(dot, -1.0f, 1.0f);

			const float angleBetween = 2.0f * std::acos(dot);
			const float t = (angleBetween <= 0.0001f) ? 1.0f : std::min(maxRadians / angleBetween, 1.0f);

			const XMVECTOR newRotation = XMQuaternionSlerp(currentRotation, targetRotation, t);
			transform.SetRotation(newRotation);
		});
}
