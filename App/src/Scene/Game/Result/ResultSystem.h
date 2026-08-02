#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include"ResultComponent.h"
#include<Scene/Game/State/GameState.h>

namespace ecs
{
	///<summary>
	///GameState::Result状態の処理を担当する。UI生成・入力処理・確定後のシーン遷移まで行う
	///</summary>
	class ResultSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		static void EnterResult(entt::registry& registry, entt::entity controllerEntity, ::sys::eResultType resultType);
		static void HandleGameOverInput(entt::registry& registry, ResultComponent& result);
	};
}
