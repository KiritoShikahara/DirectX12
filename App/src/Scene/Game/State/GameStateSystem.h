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
		// 縺薙％縺ｧ蜷・ｨｮ驕ｷ遘ｻ譎ゅ・繝｡繧ｽ繝・ラ菴懈・
		void EnterResult(::ecs::GameStateComponent& controller);
	};
}



