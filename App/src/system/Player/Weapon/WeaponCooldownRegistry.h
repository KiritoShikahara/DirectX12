#pragma once

#include<entt/entt.hpp>

namespace ecs
{
	struct WeaponComponent;
}

namespace ecs::weaponutil
{
	/// <summary>
	/// 武器エンティティの現在のクールダウン状態を取得する。
	/// 武器種別(WeaponComponent::Type)ごとにRuntimeComponent/マスタデータの型が異なるため、
	/// テーブル参照で汎用化する(WeaponTypeRegistry.cppと同じ設計方針。新しい武器種別を
	/// 追加する場合はWeaponCooldownRegistry.cppのテーブルへ1行追記するだけでよい)。
	/// </summary>
	/// <param name="outRemainingSeconds">残りクールダウン秒数(出力、0以下で発射可能)</param>
	/// <param name="outMaxSeconds">
	/// 直近に発動した際のクールダウン最大秒数(出力、FireInterval×GetCooldownRateで算出)。
	/// UIでの残量比率(outRemainingSeconds/outMaxSeconds)の計算に使う。
	/// </param>
	/// <returns>
	/// クールダウンの概念を持つ武器ならtrue。SelfDefense(Orbit、発動トリガーの無い常時稼働武器)は
	/// クールダウン自体が存在しないためfalseを返す。
	/// </returns>
	bool TryGetWeaponCooldown(
		entt::registry& registry,
		entt::entity weaponEntity,
		const WeaponComponent& weapon,
		float& outRemainingSeconds,
		float& outMaxSeconds);
}
