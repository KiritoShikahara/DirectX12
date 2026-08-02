#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///指向性ライトのシャドウ注視点をプレイヤー位置のXZへ追従させる。ShadowRangeを絞ってあるためワールド原点固定だとフィールドの大部分で影が機能しなくなるのを防ぐ
	///</summary>
	class DirLightFollowSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
