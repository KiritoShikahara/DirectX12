#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include"ResultComponent.h"
#include<Scene/Game/State/GameState.h>

namespace ecs
{
	/// <summary>
	/// GameState::Result 状態の一連の処理を担当する。
	/// - Result状態に入った最初のフレームで、クリア/ゲームオーバーに応じたUIを生成する
	/// - ゲームクリア: Select確定でタイトルへ
	/// - ゲームオーバー: MenuLeft/MenuRightでRetry/Titleを選択、Selectで確定
	/// - 確定後は SceneManager でシーン遷移する
	///
	/// GameStateSystem はステート遷移のみを担当し、リザルト固有の知識を持たせない
	/// （PerkSelectSystem と同じ疎結合の設計方針）。
	/// </summary>
	class ResultSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		static void EnterResult(entt::registry& registry, entt::entity controllerEntity, ::sys::eResultType resultType);
		static void HandleGameOverInput(entt::registry& registry, ResultComponent& result);
	};
}
