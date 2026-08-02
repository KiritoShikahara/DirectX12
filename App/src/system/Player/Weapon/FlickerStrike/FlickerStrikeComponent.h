#pragma once

#include<entt/entt.hpp>

namespace ecs
{
    ///<summary>
    ///プレイヤーのFlicker Strike発動状態を保持するランタイムコンポーネント。状態遷移はFlickerStrikeWeaponSystemが担当する
    ///</summary>
    struct PlayerFlickerStrikeComponent
    {
        ///<summary>
        ///ワープ攻撃シーケンスの実行中か
        ///</summary>
        bool IsActive = false;

        ///<summary>
        ///残りの追加ワープ攻撃回数、初撃を含まない
        ///</summary>
        int RemainingHits = 0;

        ///<summary>
        ///次のワープ攻撃までの残り時間、秒
        ///</summary>
        float WarpTimer = 0.0f;

        ///<summary>
        ///直前にワープ攻撃した対象、次のワープ先探索で除外する基準にする
        ///</summary>
        entt::entity CurrentTarget = entt::null;
    };

    ///<summary>
    ///プレイヤーがFlicker Strikeのワープシーケンス中かどうかを判定する。他の武器発動・操作の一時停止に使う
    ///</summary>
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
