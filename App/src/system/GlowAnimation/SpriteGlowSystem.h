#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{

	class SpriteGlowSystem : public IUserSystem
	{
	public: 
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	private:
		// システムの起動からの累積時間
		float mTotalTime = 0;

	};
}

