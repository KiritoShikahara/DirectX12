#pragma once

#include<entt/entt.hpp>

namespace sys
{
    ///<summary>
    ///ゲームの状態
    ///</summary>
    enum class eGameState
    {
        PreStart,
        InGame,
        PerkSelect,
        Result,
    };

    ///<summary>
    ///リザルトの種別。Result状態の時だけ有効
    ///</summary>
    enum class eResultType
    {
        None,
        Clear,
        GameOver,
    };
}

namespace ecs
{
    ///<summary>
    ///状態管理用コンポーネント
    ///</summary>
    struct GameStateComponent
    {
        // 現在の状態。GameStateSystemだけが書き換える
        ::sys::eGameState GameState = ::sys::eGameState::PreStart;

        // リザルト種別。Result遷移時にGameStateSystemが確定させる
        ::sys::eResultType ResultType = ::sys::eResultType::None;

        // 遷移リクエスト用フラグ。各Systemがtrueにし、GameStateSystemが検知して状態を進める
        int PendingLevelUpCount = 0; // InGameからPerkSelectへの遷移トリガー
        bool PerkSelectDone = false; // PerkSelectからInGameへの遷移トリガー
        bool GameClearRequested = false; // InGameからResult・Clearへの遷移トリガー
        bool GameOverRequested = false; // InGameからResult・GameOverへの遷移トリガー

        // 設定メニューが開いているかを示す。開いている間は他の入力処理を止める必要がある
        bool IsOptionsMenuOpen = false;
    };

    ///<summary>
    ///設定メニューが開いているかどうかを判定する。メニュー表示中に止めるべき入力処理はここを参照する
    ///</summary>
    inline bool IsOptionsMenuOpen(entt::registry& registry)
    {
        auto view = registry.view<GameStateComponent>();
        if (view.begin() == view.end()) return false;

        return registry.get<GameStateComponent>(*view.begin()).IsOptionsMenuOpen;
    }

}
