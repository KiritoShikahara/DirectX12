#pragma once

#include<entt/entt.hpp>

namespace ecs
{
    enum class eWeaponType;
}

namespace ecs::weaponutil
{
    /// <summary>
    /// weaponEntityへ、type(eWeaponType)に対応するランタイムコンポーネント
    /// (XxxWeaponRuntimeComponent)を1つ付与する。GameSceneFactory::AddWeaponToPlayerが
    /// 武器種別ごとのswitch文で個別にAddComponentを呼んでいたため、新規武器を追加するたびに
    /// このswitchへ1ケース追記する必要があった。テーブル参照(WeaponTypeRegistry.cpp)へ
    /// 置き換えることで、新しい武器種別の追加時に触る箇所をそちらの1箇所へ集約する。
    /// 対応するエントリが無い場合は何もしない。
    /// </summary>
    void AddWeaponRuntimeComponent(entt::registry& registry, eWeaponType type, entt::entity weaponEntity);
}
