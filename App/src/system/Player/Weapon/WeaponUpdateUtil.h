#pragma once

#include<entt/entt.hpp>

namespace ecs
{
    struct WeaponComponent;
}

namespace ecs::weaponutil
{
    /// <summary>今InGame状態かどうか(PerkSelect/Result中やGameStateComponent不在ならfalse)</summary>
    bool IsInGame(entt::registry& registry);

    /// <summary>自動発動武器(Nova/HomingMissile/ChainLightning/Meteor/Orbit/VoidBeam/
    /// BoneSpear/Cleave)のUpdate()冒頭で今フレームをスキップすべきか判定する。
    /// これらはFlicker Strike中も止めたくないので、必殺技中かどうかだけを見る</summary>
    bool ShouldSkipAutoWeaponUpdate(entt::registry& registry);

    /// <summary>手動発動武器(SingleShot/AreaAttack)のUpdate()冒頭で今フレームを
    /// スキップすべきか判定する</summary>
    bool ShouldSkipManualWeaponUpdate(entt::registry& registry);

    /// <summary>武器マスタデータの検索ID((WeaponID+1)*1000+Level)を計算する</summary>
    int ComputeWeaponDataId(const ecs::WeaponComponent& weapon);
}
