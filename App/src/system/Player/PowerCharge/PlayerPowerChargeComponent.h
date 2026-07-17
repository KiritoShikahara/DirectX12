#pragma once

namespace ecs
{
    /// <summary>
    /// プレイヤーが所持するパワーチャージ数。敵撃破ごとに加算され(EnemyDeathSystem::
    /// AwardPowerCharge参照)、Flicker Strike等のチャージ消費スキルで全消費される。
    /// 上限(MaxCharge)はFlickerStrikeWeaponDataで管理する(現状の唯一の消費先のため。
    /// 将来チャージを使う武器が増えた場合は共有のコンフィグへ切り出す)。
    /// 時間経過による自然減衰は行わない(貯めたら消費するまで維持する)。
    /// </summary>
    struct PlayerPowerChargeComponent
    {
        int Count = 0;
    };
}
