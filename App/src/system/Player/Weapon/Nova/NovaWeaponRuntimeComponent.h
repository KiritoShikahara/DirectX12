#pragma once

namespace ecs
{
    /// <summary>
    /// Nova型武器（WeaponComponent::Type == Nova）専用のランタイム状態。
    /// クールダウン等、武器種別ごとに異なる実行時データは WeaponComponent(共通情報)には
    /// 持たせず、種別ごとのコンポーネントに分離する
    /// （新しい武器種別を追加してもここには影響しない）。
    /// </summary>
    struct NovaWeaponRuntimeComponent
    {
        /// <summary>次の発動までの残り時間(秒)。0以下で発動可能</summary>
        float CooldownTimer = 0.0f;
    };
}
