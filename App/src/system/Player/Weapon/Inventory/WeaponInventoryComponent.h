#pragma once

#include<vector>
#include<entt/entt.hpp>

namespace ecs
{
	/// <summary>
	/// プレイヤーが所持する武器の管理をするコンポーネント。
    /// スロット上限を超えて新規武器を取得することはできない。
    /// （上限到達後はレベルアップのみ可能とする想定）
	/// </summary>
	struct WeaponInventoryComponent
	{
        /// <summary>所持している武器エンティティ（生成順）</summary>
        std::vector<entt::entity> Weapons;

        /// <summary>同時所持できる武器の最大数</summary>
        int MaxSlots = 6;

        /// <summary>現在の所持数</summary>
        int Count() const { return static_cast<int>(Weapons.size()); }

        /// <summary>新規武器を取得できるか（空きスロットがあるか）</summary>
        bool HasFreeSlot() const { return Count() < MaxSlots; }
	};
}