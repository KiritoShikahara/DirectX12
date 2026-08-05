#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
	///<summary>
	///StartEffectComponentが付いたテキストのフェードイン→維持→フェードアウトを進行させ、
	///完了したら自動で破棄する
	///</summary>
	class StartEffectSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>フェードアウトが完了し今フレームで破棄する対象。毎フレーム使い回す</summary>
		std::vector<entt::entity> mExpired;
	};
}
