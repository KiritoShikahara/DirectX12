#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// LoadingScreenComponentを持つエンティティを毎フレーム処理する。
	/// スピナー(ローディングアイコン)を回転させ、バックグラウンドスレッドの
	/// 先読み完了(IsComplete)を検知した瞬間にOnCompleteを1回だけ呼ぶ
	/// (実際のシーン切り替えはOnComplete側=呼び出し元が担当する)。
	/// </summary>
	class LoadingScreenUpdateSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
