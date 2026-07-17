#include "apppch.h"
#include "PlayerRegenSystem.h"

#include<system/Player/Status/PlayerStatusComponent.h>
#include<Tag/EntityTag.h>

#include<algorithm>

namespace ecs
{
	void PlayerRegenSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		registry.view<PlayerTag, PlayerStatusComponent>().each(
			[&](PlayerStatusComponent& status)
			{
				if (status.Current.HpRegenPerSecond <= 0.0f) return;
				if (status.CurrentHp <= 0.0f) return; // 死亡後は回復させない

				status.CurrentHp = std::min(
					status.Current.MaxHp,
					status.CurrentHp + status.Current.HpRegenPerSecond * deltaTime);
			});
	}
}
