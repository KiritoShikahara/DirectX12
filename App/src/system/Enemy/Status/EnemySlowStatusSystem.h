#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
	///<summary>
	///EnemySlowStatusComponentの残り時間を毎フレーム減らし、尽きたコンポーネントを外して通常速度へ戻すシステム
	///</summary>
	class EnemySlowStatusSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///期限切れエンティティの一時バッファ。走査完了後にまとめて外すため、また毎フレームのvector生成を避けるためメンバで使い回す
		///</summary>
		std::vector<entt::entity> mExpired;
	};
}
