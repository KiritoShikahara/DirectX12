#include "apppch.h"
#include "GameStateSystem.h"

#include"GameState.h"
#include<Scene/Game/Factory/GameSceneFactory.h>

namespace sys
{
	void GameStateSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
        // 管理コンポーネントの存在判定
        auto controllerView = registry.view<::ecs::GameStateComponent>();
        if (controllerView.size() <= 0)
        {
            return;
        }
        // 最初のコンポーネントだけ取得
        auto& controller = controllerView.get<::ecs::GameStateComponent>(controllerView.front());

        // 状態遷移
        switch (controller.GameState)
        {
        case eGameState::PreStart:
            // トランジション終了でゲーム開始
            if (::sys::SceneManager::Get().IsTransitionFinished() == true)
            {
                // 開始エフェクト
                ::ecs::GameSceneFactory::CreateStartEffect();
                // 開始
                controller.GameState = eGameState::InGame;
                // タイムスケールを戻す
                GetTime().SetTimeScale(1.0);
            }
            break;

        case eGameState::InGame:
            // ゲームオーバー
            if (controller.GameOverRequested)
            {
                controller.GameOverRequested = false;
                controller.ResultType = eResultType::GameOver;
                EnterResult(controller);
                break;
            }

            // ゲームクリア
            if (controller.GameClearRequested)
            {
                controller.GameClearRequested = false;
                controller.ResultType = eResultType::Clear;
                EnterResult(controller);
                break;
            }

            // レベルアップ → パーク選択
            if (controller.LevelUpRequested)
            {
                controller.LevelUpRequested = false;
                // TODO:GameSceneFactory::CreatePerkSelectUI();
                controller.GameState = eGameState::PerkSelect;
                GetTime().SetTimeScale(0.0);
            }
            break;

        case eGameState::PerkSelect:
            // パーク選択完了 → ゲームに戻る
            if (controller.PerkSelectDone)
            {
                controller.PerkSelectDone = false;
                controller.GameState = eGameState::InGame;
                GetTime().SetTimeScale(1.0);
            }
            break;

        case eGameState::Result:
            break;

        }
	}

    void GameStateSystem::EnterResult(::ecs::GameStateComponent& controller)
    {
        controller.GameState = eGameState::Result;
        // TODO:リザルトのウィジェット生成メソッドを呼び出す
    }
}
