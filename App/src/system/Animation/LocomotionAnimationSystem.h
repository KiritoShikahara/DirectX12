#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
	///<summary>
	///移動状態に応じてFbxAnimComponentの再生クリップをIdle/Runへ切り替えるシステム。ActionAnimLockComponentが付いている間は切り替えを止める
	///</summary>
	class LocomotionAnimationSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///ActionAnimLockComponentの期限切れエンティティの一時バッファ。走査完了後にまとめて外すため、また毎フレームのvector生成を避けるためメンバで使い回す
		///</summary>
		std::vector<entt::entity> mExpiredLocks;
	};
}
