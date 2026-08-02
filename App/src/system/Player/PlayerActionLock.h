#pragma once

#include<entt/entt.hpp>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeComponent.h>

namespace ecs
{
    /// <summary>
    /// プレイヤーが「完全に排他的な演出中」(必殺技のワープ/ビーム演出、Flicker Strikeの
    /// ワープ攻撃シーケンス等、入力そのものを受け付けるべきでない状態)かどうかを判定する。
    /// `PlayerInputSystem`(入力そのものを止める)・`GameStateSystem`(パーク選択への遷移保留)・
    /// 手動発動武器(SingleShot/AreaAttack、Attack/Attack2で撃つ武器)はこちらを参照する。
    ///
    /// 一方、狙い不要で自動発動する武器(Nova/Homing Missile/Chain Lightning/Meteor/
    /// SelfDefense(Orbit)/Void Beam/Bone Spear/Cleaveと、それらが使う共有のProjectile系
    /// システム)は、Flicker Strike中も止めない(ユーザー指示：「フリッカーストライク中に
    /// オートの攻撃も行われなくなっているのでオートの攻撃は行うように」)。これらは
    /// `IsPlayerActionLocked()`ではなく`IsPlayerUltimateActive()`を直接参照し、必殺技演出中のみ
    /// 停止する。手動攻撃(Attack/Attack2)の入力自体はFlicker Strike中の
    /// `FlickerStrikeWeaponSystem::UpdateActiveSequence`が検知してシーケンスを打ち切る
    /// (手動攻撃側はPlayerInputSystemの入力ロックにより実際には発射されない)。
    ///
    /// 今後同種の完全排他スキルを追加する場合はここに条件を1つ足すだけでよい
    /// (以前はIsPlayerUltimateActive()のみを15箇所で個別に呼んでいたが、2つ目の排他スキル
    /// (Flicker Strike)を追加するにあたり、同じガードを2重に書き並べる重複を避けるため統合した)。
    /// </summary>
    inline bool IsPlayerActionLocked(entt::registry& registry)
    {
        return IsPlayerUltimateActive(registry) || IsPlayerFlickerStrikeActive(registry);
    }
}
