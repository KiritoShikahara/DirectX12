#pragma once

#include<entt/entt.hpp>

namespace ecs
{
    struct WeaponComponent;
}

namespace ecs::weaponutil
{
    ///<summary>
    ///今InGame状態かどうか、PerkSelect/Result中やGameStateComponent不在ならfalse
    ///</summary>
    bool IsInGame(entt::registry& registry);

    ///<summary>
    ///自動発動武器のUpdate冒頭で今フレームをスキップすべきか判定する。必殺技中かどうかだけを見る
    ///</summary>
    bool ShouldSkipAutoWeaponUpdate(entt::registry& registry);

    ///<summary>
    ///手動発動武器のUpdate冒頭で今フレームをスキップすべきか判定する
    ///</summary>
    bool ShouldSkipManualWeaponUpdate(entt::registry& registry);

    ///<summary>
    ///武器マスタデータの検索IDを計算する、WeaponID+1を1000倍してLevelを加算した値
    ///</summary>
    int ComputeWeaponDataId(const ecs::WeaponComponent& weapon);
}
