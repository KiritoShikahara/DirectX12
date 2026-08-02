#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{

	class SpriteGlowSystem : public IUserSystem
	{
	public: 
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	private:
		// 繧ｷ繧ｹ繝・Β縺ｮ襍ｷ蜍輔°繧峨・邏ｯ遨肴凾髢・
		float mTotalTime = 0;

	};
}

