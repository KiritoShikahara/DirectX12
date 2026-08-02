#pragma once

namespace ecs
{
    ///<summary>
    ///FlickerStrike型武器専用のランタイム状態。ワープシーケンス自体はプレイヤー側のPlayerFlickerStrikeComponentが持つ
    ///</summary>
    struct FlickerStrikeRuntimeComponent
    {
        ///<summary>
        ///次の発動までの残り時間、秒。0以下で発動可能
        ///</summary>
        float CooldownTimer = 0.0f;
    };
}
