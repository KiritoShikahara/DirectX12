#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// ハブ画面(HubScene)の選択肢移動・確定・キャンセルを処理する。
	/// - MenuLeft/MenuRightでカーソル移動
	/// - Selectで確定：選択肢に応じてMenuScene(武器・ステージ選択)/StatusUpgradeScene
	///   (ステータス強化)へ遷移する
	/// - CancelでTitleSceneへ戻る
	/// </summary>
	class HubMenuInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
