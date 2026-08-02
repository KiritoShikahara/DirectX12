#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>

namespace ecs
{
	struct PlayerStateComponent;

	///<summary>
	///プレイヤーの状態管理
	///</summary>
	class PlayerStateSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		void ResolveRequests(ecs::PlayerStateComponent& state);
	};
}


