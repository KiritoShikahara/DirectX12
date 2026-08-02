#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///必殺技ゲージUIの充填率を更新するシステム。PlayerUltimateComponent::KillCountをUltimateData::RequiredKillCountで割った比率をPlayerUltimateGaugeTagのSprite::FillAmountへ反映する
	///</summary>
	class PlayerUltimateGaugeSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
