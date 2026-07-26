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
				// Y(高さ)は既定値(地面レベル)のまま固定する。フィールドはほぼ平坦なため、
				// ライト空間の奥行き(Near/Far)方向はプレイヤーの高さ変化を考慮する必要が薄く、
				// XZだけプレイヤーへ追従させれば正射影の範囲(ShadowRange)を常にプレイヤー
				// 中心に保てる。
				light.ShadowTarget.x = playerPos.x;
				light.ShadowTarget.z = playerPos.z;
			});
	}
}
