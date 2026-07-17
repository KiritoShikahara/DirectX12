#pragma once

#include<entt/entt.hpp>

namespace ecs
{
    /// <summary>
    /// プレイヤーのFlicker Strike発動状態を保持するランタイムコンポーネント。
    /// 状態遷移(初撃→ワープ→終了)はFlickerStrikeWeaponSystemが担当する
    /// (PlayerUltimateComponentと同じ「プレイヤー本体に持たせる演出状態」の方針)。
    /// </summary>
    struct PlayerFlickerStrikeComponent
    {
        /// <summary>ワープ攻撃シーケンスの実行中か</summary>
        bool IsActive = false;

        /// <summary>残りの追加ワープ攻撃回数(初撃を含まない)</summary>
        int RemainingHits = 0;

        /// <summary>次のワープ攻撃までの残り時間(秒)</summary>
        float WarpTimer = 0.0f;

        /// <summary>直前にワープ攻撃した対象(次のワープ先探索で除外する基準にする)</summary>
        entt::entity CurrentTarget = entt::null;
    };

    /// <summary>
    /// プレイヤーがFlicker Strikeのワープシーケンス中(IsActive)かどうかを判定する。
    /// IsPlayerUltimateActive()と同じ使い方(他の武器の発動・プレイヤー操作の一時停止に使う)。
    /// 単独では使わず、通常はecs::IsPlayerActionLocked()(PlayerActionLock.h)経由で参照する。
    /// </summary>
    inline bool IsPlayerFlickerStrikeActive(entt::registry& registry)
    {
        bool isActive = false;
        registry.view<PlayerFlickerStrikeComponent>().each(
            [&](const PlayerFlickerStrikeComponent& flicker)
            {
                if (flicker.IsActive) isActive = true;
            });
        return isActive;
    }
}
