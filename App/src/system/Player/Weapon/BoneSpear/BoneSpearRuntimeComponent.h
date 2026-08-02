#pragma once

namespace ecs
{
    ///<summary>
    ///BoneSpear型武器専用のランタイム状態、クールダウン等
    ///</summary>
    struct BoneSpearRuntimeComponent
    {
        ///<summary>
        ///次の発射までの残り時間、秒。0以下で発射可能
        ///</summary>
        float CooldownTimer = 0.0f;
    };
}
