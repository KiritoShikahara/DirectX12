#include "apppch.h"
#include "TitleInputSystem.h"
#include <Scene/Hub/HubScene.h>

void sys::TitleInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{

	auto& input = ::sys::InputManager::Get();

	// セレクトなら SE + ハブ画面遷移（武器・ステージ選択/ステータス強化の2択はHubSceneが担当）
	if (input.IsActionPressed("Select") == true)
	{
		::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::HubScene>();
		PLAY_SE("Assets/Sound/SE/SE_Select.aud",false,1,false);
	}

	// 終了なら SE + 確認画面

	// 終了のシステムは全画面で共通できるように別システムとして存在。
	// 専用のコンポーネントの存在で処理分岐をするようにする。

}
