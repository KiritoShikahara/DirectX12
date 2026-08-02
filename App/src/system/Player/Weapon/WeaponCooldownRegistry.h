#pragma once

#include<entt/entt.hpp>

namespace ecs
{
	struct WeaponComponent;
}

namespace ecs::weaponutil
{
	///<summary>
	///武器エンティティの現在のクールダウン状態を取得する。武器種別ごとにRuntimeComponent/マスタデータの型が異なるためテーブル参照で汎用化する
	///</summary>
	/// <param name="outRemainingSeconds">残りクールダウン秒数、出力。0以下で発射可能</param>
	/// <param name="outMaxSeconds">直近に発動した際のクールダウン最大秒数、出力。UIでの残量比率の計算に使う</param>
	/// <returns>クールダウンの概念を持つ武器ならtrue。SelfDefenseのOrbitは発動トリガーの無い常時稼働武器のためfalseを返す</returns>
	bool TryGetWeaponCooldown(
		entt::registry& registry,
		entt::entity weaponEntity,
		const WeaponComponent& weapon,
		float& outRemainingSeconds,
		float& outMaxSeconds);
}
