#include "apppch.h"
#include "GameStateSystem.h"

#include"GameStateComponent.h"
#include<Scene/Game/Factory/GameSceneFactory.h>

namespace ecs
{
	void GameStateSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto controllerView = registry.view<GameStateComponent>();
		if (controllerView.size() <= 0)
		{
			return;
		}

		auto& controller = controllerView.get<GameStateComponent>(controllerView.front());

		switch (controller.GameState)
		{
		case eGameState::PreStart:
			// トランジション終了判定
			if (::sys::SceneManager::Get().IsTransitionFinished() == true)
			{
				// スタート演出用のコンポーネントの追加
				GameSceneFactory::CreateStartEffect();
				// 状態変更
				controller.GameState = eGameState::InGame;
			}
			break;

		case eGameState::InGame:
			break;

		case eGameState::PerkSelect:
			break;

		case eGameState::Result:
			break;

		default:
			break;
		}

	}
}
