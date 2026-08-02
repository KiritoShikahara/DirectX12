#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///LoadingScreenComponentを持つエンティティを毎フレーム処理する。スピナーを回転させ、先読み完了を検知した瞬間にOnCompleteを1回だけ呼ぶ
	///</summary>
	class LoadingScreenUpdateSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
