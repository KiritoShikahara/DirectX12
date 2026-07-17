#include "apppch.h"
#include "GameStateSystem.h"

#include"GameState.h"
#include<Scene/Game/Factory/GameSceneFactory.h>
#include<system/Player/PlayerActionLock.h>

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

            // レベルアップ → パーク選択（必殺技/Flicker Strike演出中は演出が終わるまで遷移を保留する）
            if (controller.PendingLevelUpCount > 0 && !::ecs::IsPlayerActionLocked(registry))
            {
                // UI生成・入力・効果適用は PerkSelectSystem が状態を見て自律的に行う
                // （GameStateSystem はステート遷移のみを担当し、パーク固有の知識を持たない）
                controller.GameState = eGameState::PerkSelect;
                GetTime().SetTimeScale(0.0);
            }
            break;

        case eGameState::PerkSelect:
            // パーク選択完了 → 未消化のレベルアップが残っていれば続けてもう1回パーク選択を
            // 提示する（PerkSelectComponentを外したままPerkSelect状態に留まると、
            // PerkSelectSystemが次フレームで自動的に新しい3択を生成する）。
            // 残っていなければゲームに戻る。
            if (controller.PerkSelectDone)
            {
                controller.PerkSelectDone = false;
                if (controller.PendingLevelUpCount > 0)
                {
                    controller.PendingLevelUpCount -= 1;
                }

                if (controller.PendingLevelUpCount <= 0)
                {
                    controller.GameState = eGameState::InGame;
                    GetTime().SetTimeScale(1.0);
                }
            }
            break;

        case eGameState::Result:
            break;

        }
	}

    void GameStateSystem::EnterResult(::ecs::GameStateComponent& controller)
    {
        controller.GameState = eGameState::Result;
        GetTime().SetTimeScale(0.0);
        // UI生成・入力・シーン遷移は ResultSystem が状態を見て自律的に行う
        // （GameStateSystem はステート遷移のみを担当し、リザルト固有の知識を持たない）
    }
}
