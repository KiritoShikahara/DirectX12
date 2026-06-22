#include "apppch.h"
#include "TitleInputSystem.h"
#include <Scene/Menu/MenuScene.h>

void sys::TitleInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{

	auto& input = ::sys::InputManager::Get();

	// セレクトなら SE + メニュー画面遷移
	if (input.IsActionPressed("Select") == true)
	{
		::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::MenuScene>();
	}

	// 終了なら SE + 確認画面

	// 終了のシステムは全画面で共通できるように別システムとして存在。
	// 専用のコンポーネントの存在で処理分岐をするようにする。

}
