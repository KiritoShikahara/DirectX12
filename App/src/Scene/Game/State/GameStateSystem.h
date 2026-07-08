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
		// ここで各種遷移時のメソッド作成
		void EnterResult(::ecs::GameStateComponent& controller);
	};
}



