#pragma once

namespace ecs
{
    /// <summary>
    /// BoneSpear型武器（WeaponComponent::Type == BoneSpear）専用のランタイム状態。
    /// クールダウン等、武器種別ごとに異なる実行時データは WeaponComponent(共通情報)には
    /// 持たせず、種別ごとのコンポーネントに分離する
    /// （SingleShotWeaponRuntimeComponentと同じ方針）。
    /// </summary>
    struct BoneSpearRuntimeComponent
    {
        /// <summary>次の発射までの残り時間(秒)。0以下で発射可能</summary>
        float CooldownTimer = 0.0f;
    };
}
