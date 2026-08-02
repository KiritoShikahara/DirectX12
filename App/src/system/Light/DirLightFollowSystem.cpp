#include "apppch.h"
#include "DirLightFollowSystem.h"

#include<Tag/EntityTag.h>
#include<ecs/component/Light/LightComponent.h>

namespace ecs
{
	void DirLightFollowSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto playerView = registry.view<PlayerTag, Transform>();
		if (playerView.begin() == playerView.end()) return;

		const DirectX::XMFLOAT3& playerPos = registry.get<Transform>(*playerView.begin()).GetPosition();

		registry.view<DirectionalLightComponent>().each(
			[&](DirectionalLightComponent& light)
			{
				// Yは地面レベルで固定。フィールドはほぼ平坦なためXZだけプレイヤーへ追従させればShadowRangeを常にプレイヤー中心に保てる
				light.ShadowTarget.x = playerPos.x;
				light.ShadowTarget.z = playerPos.z;
			});
	}
}
