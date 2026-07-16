#pragma once

#include<vector>
#include<entt/entt.hpp>

namespace ecs
{
	/// <summary>
	/// SelfDefense(周回)型武器（WeaponComponent::Type == SelfDefense）専用のランタイム状態。
	/// 発動トリガーが無く常時稼働する武器のため、クールダウンではなく
	/// 生成済みのオーブエンティティ一覧を保持する
	/// （AreaAttackWeaponRuntimeComponentと同じ方針で、種別ごとに分離する）。
	/// </summary>
	struct OrbitWeaponRuntimeComponent
	{
		/// <summary>周回中のオーブエンティティ（初回Updateで生成、以後は空にならない想定）</summary>
		std::vector<entt::entity> Orbs;
	};
}
