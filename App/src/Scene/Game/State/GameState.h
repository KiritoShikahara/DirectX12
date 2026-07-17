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

        // 未消化のレベルアップ回数。1回のXP付与で複数レベル分の閾値を同時に超えた場合も
        // レベルアップした回数分だけパーク選択を連続で提示するため、bool ではなく回数で持つ
        // (EnemyDeathSystemが加算、GameStateSystemがPerkSelect 1回完了ごとに1減算する)。
        // 必殺技/Flicker Strike演出中はIsPlayerActionLocked()でInGame→PerkSelectの遷移を保留する。
        int PendingLevelUpCount = 0; // InGame → PerkSelect
        bool PerkSelectDone = false; // PerkSelect → InGame (もしくはPerkSelectのまま次の1回へ)
        bool GameClearRequested = false; // InGame → Result(Clear)
        bool GameOverRequested = false; // InGame → Result(GameOver)
    };

}
