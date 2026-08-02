#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	struct GameStateComponent;
}

namespace sys
{
	class GameStateSystem : public ::ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime);

	private:
		///<summary>
		///Result状態への遷移処理
		///</summary>
		void EnterResult(::ecs::GameStateComponent& controller);
	};
}



