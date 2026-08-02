#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///経験値バー・現在レベル表示のUIを更新するシステム。PlayerLevelComponent::Experience/ExperienceToNextLevelの比率をPlayerExpBarTagのSprite::FillAmountへ反映し、PlayerLevelTextTagへ現在レベルを表示する
	///</summary>
	class PlayerExpBarSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
