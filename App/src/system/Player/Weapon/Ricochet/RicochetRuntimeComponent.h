#pragma once

namespace ecs
{
    ///<summary>
    ///Ricochet型武器専用のランタイム状態、クールダウン等
    ///</summary>
    struct RicochetRuntimeComponent
    {
        ///<summary>
        ///次の発射までの残り時間、秒。0以下で発射可能
        ///</summary>
        float CooldownTimer = 0.0f;
    };
}
