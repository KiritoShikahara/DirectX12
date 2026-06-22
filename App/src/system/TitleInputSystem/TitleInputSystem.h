#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace sys
{
	/// <summary>
	/// 入力されたらタイトル画面から次の画面に遷移する。
	/// </summary>
	class TitleInputSystem : public ::ecs::IUserSystem
	{
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}


