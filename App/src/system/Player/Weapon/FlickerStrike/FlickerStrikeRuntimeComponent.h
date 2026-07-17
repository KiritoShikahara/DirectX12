#pragma once

namespace ecs
{
    /// <summary>
    /// FlickerStrike型武器（WeaponComponent::Type == FlickerStrike）専用のランタイム状態。
    /// クールダウン等、武器種別ごとに異なる実行時データは WeaponComponent(共通情報)には
    /// 持たせず、種別ごとのコンポーネントに分離する
    /// （SingleShotWeaponRuntimeComponentと同じ方針）。
    /// ワープシーケンス自体の状態は武器エンティティ側ではなくプレイヤー側の
    /// PlayerFlickerStrikeComponentが持つ(発動後も所有武器の存在に依存させないため)。
    /// </summary>
    struct FlickerStrikeRuntimeComponent
    {
        /// <summary>次の発動までの残り時間(秒)。0以下で発動可能</summary>
        float CooldownTimer = 0.0f;
    };
}
