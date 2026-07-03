#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>

namespace ecs
{
	struct PlayerStateComponent;

	/// <summary>
	/// ƒvƒŒƒCƒ„[‚Ìó‘ÔŠÇ—
	/// </summary>
	class PlayerStateSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		void ResolveRequests(ecs::PlayerStateComponent& state);
	};
}


