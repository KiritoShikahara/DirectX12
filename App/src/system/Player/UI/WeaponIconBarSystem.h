#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///所持武器アイコンバーの表示内容を毎フレーム更新するシステム。武器所持状況に応じたアイコン表示、クールダウン進捗のオーバーレイ、残り秒数テキストを反映する
	///</summary>
	class WeaponIconBarSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
