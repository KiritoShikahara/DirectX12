#pragma once

namespace ecs
{
    ///<summary>
    ///Chain Lightning型武器専用のランタイム状態、クールダウン等
    ///</summary>
    struct ChainLightningRuntimeComponent
    {
        ///<summary>
        ///次の発動までの残り時間、秒。0以下で発動可能
        ///</summary>
        float CooldownTimer = 0.0f;
    };
}
