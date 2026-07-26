#pragma once

namespace sys
{
    /// <summary>
    /// 繧ｲ繝ｼ繝迥ｶ諷・
    /// </summary>
    enum class eGameState
    {
        PreStart,
        InGame,
        PerkSelect,
        Result,
    };

    /// <summary>
    /// 繝ｪ繧ｶ繝ｫ繝医・遞ｮ蛻･・・esult 迥ｶ諷九・縺ｨ縺阪・縺ｿ譛牙柑・・
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
    /// 迥ｶ諷狗ｮ｡逅・さ繝ｳ繝昴・繝阪Φ繝・
    /// </summary>
    struct GameStateComponent
    {
        // 迴ｾ蝨ｨ迥ｶ諷具ｼ・ameStateSystem 縺ｮ縺ｿ縺梧嶌縺肴鋤縺医ｋ・・
        ::sys::eGameState GameState = ::sys::eGameState::PreStart;

        // 繝ｪ繧ｶ繝ｫ繝育ｨｮ蛻･・・esult 騾ｲ蜈･譎ゅ↓ GameStateSystem 縺檎｢ｺ螳壹＆縺帙ｋ・・
        ::sys::eResultType ResultType = ::sys::eResultType::None;

        // 驕ｷ遘ｻ繝ｪ繧ｯ繧ｨ繧ｹ繝・

        // 譛ｪ豸亥喧縺ｮ繝ｬ繝吶Ν繧｢繝・・蝗樊焚縲・蝗槭・XP莉倅ｸ弱〒隍・焚繝ｬ繝吶Ν蛻・・髢ｾ蛟､繧貞酔譎ゅ↓雜・∴縺溷ｴ蜷医ｂ
        // 繝ｬ繝吶Ν繧｢繝・・縺励◆蝗樊焚蛻・□縺代ヱ繝ｼ繧ｯ驕ｸ謚槭ｒ騾｣邯壹〒謠千､ｺ縺吶ｋ縺溘ａ縲｜ool 縺ｧ縺ｯ縺ｪ縺丞屓謨ｰ縺ｧ謖√▽
        // (EnemyDeathSystem縺悟刈邂励；ameStateSystem縺訓erkSelect 1蝗槫ｮ御ｺ・＃縺ｨ縺ｫ1貂帷ｮ励☆繧・縲・
        // 蠢・ｮｺ謚/Flicker Strike貍泌・荳ｭ縺ｯIsPlayerActionLocked()縺ｧInGame竊単erkSelect縺ｮ驕ｷ遘ｻ繧剃ｿ晉蕗縺吶ｋ縲・
        int PendingLevelUpCount = 0; // InGame 竊・PerkSelect
        bool PerkSelectDone = false; // PerkSelect 竊・InGame (繧ゅ＠縺上・PerkSelect縺ｮ縺ｾ縺ｾ谺｡縺ｮ1蝗槭∈)
        bool GameClearRequested = false; // InGame 竊・Result(Clear)
        bool GameOverRequested = false; // InGame 竊・Result(GameOver)
    };

}
