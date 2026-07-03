#include "apppch.h"
#include "PlayerMovementSystem.h"

#include"PlayerMovementComponent.h"

namespace ecs
{
	void PlayerMovementSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
	}

	DirectX::XMFLOAT3 PlayerMovementSystem::ComputeHorizontalVelocity(ecs::PlayerMovementComponent& movement, float deltaTime)
	{
		return DirectX::XMFLOAT3();
	}
}