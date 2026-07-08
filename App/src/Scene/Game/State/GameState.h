#pragma once

namespace sys
{
    /// <summary>
    /// ゲーム状態
    /// </summary>
    enum class eGameState
    {
        PreStart,
        InGame,
        PerkSelect,
        Result,
    };

    /// <summary>
    /// リザルトの種別（Result 状態のときのみ有効）
    /// </summary>
    enum class eResultType
    {
        None,
        Clear,
        GameOver,
    };
}

namespace ecs
{
    /// <summary>
    /// 状態管理コンポーネント
    /// </summary>
    struct GameStateComponent
    {
        // 現在状態（GameStateSystem のみが書き換える）
        ::sys::eGameState GameState = ::sys::eGameState::PreStart;

        // リザルト種別（Result 進入時に GameStateSystem が確定させる）
        ::sys::eResultType ResultType = ::sys::eResultType::None;

        // 遷移リクエスト
        bool LevelUpRequested = false; // InGame → PerkSelect
        bool PerkSelectDone = false; // PerkSelect → InGame
        bool GameClearRequested = false; // InGame → Result(Clear)
        bool GameOverRequested = false; // InGame → Result(GameOver)
    };

}
