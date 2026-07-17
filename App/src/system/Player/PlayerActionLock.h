#pragma once

#include<entt/entt.hpp>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeComponent.h>

namespace ecs
{
    /// <summary>
    /// プレイヤーが「排他的な演出中」(必殺技のワープ/ビーム演出、Flicker Strikeの
    /// ワープ攻撃シーケンス等、操作を受け付けず他の攻撃も一時停止すべき状態)かどうかを判定する。
    /// 各武器System・PlayerInputSystem・GameStateSystem(パーク選択への遷移保留)は、
    /// 個別にIsPlayerUltimateActive()/IsPlayerFlickerStrikeActive()を呼ぶのではなく
    /// 必ずこちらを参照すること。今後同種の排他スキルを追加する場合はここに条件を1つ足すだけでよい
    /// (以前はIsPlayerUltimateActive()のみを15箇所で個別に呼んでいたが、2つ目の排他スキル
    /// (Flicker Strike)を追加するにあたり、同じガードを2重に書き並べる重複を避けるため統合した)。
    /// </summary>
    inline bool IsPlayerActionLocked(entt::registry& registry)
    {
        return IsPlayerUltimateActive(registry) || IsPlayerFlickerStrikeActive(registry);
    }
}
