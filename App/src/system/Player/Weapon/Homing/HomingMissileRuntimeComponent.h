#pragma once

namespace ecs
{
    ///<summary>
    ///Homing Missile型武器専用のランタイム状態。武器種別ごとに異なる実行時データはここに分離する
    ///</summary>
    struct HomingMissileRuntimeComponent
    {
        ///<summary>
        ///次の発射までの残り時間、秒。0以下で発射可能
        ///</summary>
        float CooldownTimer = 0.0f;
    };
}
