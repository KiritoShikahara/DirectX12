#pragma once

#include<entt/entt.hpp>

namespace ecs
{
    enum class eWeaponType;
}

namespace ecs::weaponutil
{
    /// <summary>
    /// weaponEntityへtypeに対応するランタイムコンポーネントを1つ付与する。
    /// 新しい武器種別を追加するときはWeaponTypeRegistry.cppのテーブルに1行足すだけでよい。
    /// 対応エントリが無ければ何もしない。
    /// </summary>
    void AddWeaponRuntimeComponent(entt::registry& registry, eWeaponType type, entt::entity weaponEntity);
}
