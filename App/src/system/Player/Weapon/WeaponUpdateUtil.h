#pragma once

#include<entt/entt.hpp>

namespace ecs
{
    struct WeaponComponent;
}

namespace ecs::weaponutil
{
    /// <summary>
    /// 現在GameStateComponentがInGame状態かどうかを返す(PerkSelect/Result中や、
    /// GameStateComponentが1つも存在しない場合はfalse)。
    /// </summary>
    bool IsInGame(entt::registry& registry);

    /// <summary>
    /// 狙い不要で自動発動する武器(Nova/HomingMissile/ChainLightning/Meteor/
    /// SelfDefense(Orbit)/VoidBeam/BoneSpear/Cleaveと、それらが使う共有のProjectile系
    /// システム)のUpdate()冒頭で「今フレームは処理をスキップすべきか」を判定する
    /// (InGame中でない、または必殺技演出中)。これらの武器はFlicker Strike中は止めないため、
    /// IsPlayerActionLocked()ではなくIsPlayerUltimateActive()を使う(PlayerActionLock.h参照)。
    /// </summary>
    bool ShouldSkipAutoWeaponUpdate(entt::registry& registry);

    /// <summary>
    /// 手動発動武器(SingleShot/AreaAttack、Attack/Attack2で撃つ武器)のUpdate()冒頭で
    /// 「今フレームは処理をスキップすべきか」を判定する(InGame中でない、またはIsPlayerActionLocked())。
    /// </summary>
    bool ShouldSkipManualWeaponUpdate(entt::registry& registry);

    /// <summary>
    /// 武器マスタデータの検索ID(= (WeaponID+1)*1000+Level)を計算する。全てのXxxWeaponSystemが
    /// 同一の式を個別に持っていたため共通化した(将来IDスキームを変更する際の修正箇所を
    /// 1箇所に集約する)。
    /// </summary>
    int ComputeWeaponDataId(const ecs::WeaponComponent& weapon);
}
