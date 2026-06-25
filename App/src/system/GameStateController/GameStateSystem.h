#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	class GameStateSystem : public IUserSystem
	{
	public:
		void Update(entt::registry & registry, float deltaTime, float rawDeltaTime) override;
	};
}



