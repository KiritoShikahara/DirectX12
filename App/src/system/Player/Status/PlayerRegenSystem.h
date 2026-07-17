#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// PlayerStatusComponent::Current.HpRegenPerSecondに応じて毎フレームCurrentHpを回復する。
	/// HP自然回復強化(data::eStatUpgradeType::HpRegen)でBaseへ加算された値のみを扱う
	/// （現状パークによる倍率は無い。GameSceneFactory::ApplyStatUpgrades参照）。
	/// </summary>
	class PlayerRegenSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
