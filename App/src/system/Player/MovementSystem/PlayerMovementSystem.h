#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct PlayerMovementComponent; }

namespace ecs
{
	class PlayerMovementSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		static DirectX::XMFLOAT3 ComputeHorizontalVelocity(
			ecs::PlayerMovementComponent& movement, float deltaTime, float speedMultiplier);
	};
}


