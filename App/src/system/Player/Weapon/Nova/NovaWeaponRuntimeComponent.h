#pragma once

namespace ecs
{
    ///<summary>
    ///Nova型武器専用のランタイム状態。武器種別ごとに異なる実行時データはここに分離する
    ///</summary>
    struct NovaWeaponRuntimeComponent
    {
        ///<summary>
        ///次の発動までの残り時間、秒。0以下で発動可能
        ///</summary>
        float CooldownTimer = 0.0f;
    };
}
