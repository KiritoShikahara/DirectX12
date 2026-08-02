#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///PlayerStatusComponent::Current.HpRegenPerSecondに応じて毎フレームCurrentHpを回復する
	///</summary>
	class PlayerRegenSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
