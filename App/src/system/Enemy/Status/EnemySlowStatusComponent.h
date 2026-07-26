#pragma once

namespace ecs
{
	/// <summary>
	/// 敵の移動速度減速デバフ(スロウ)の実行時状態。効果が掛かっている敵にのみ付与する
	/// (EnemyChaseSystemがこのコンポーネントの有無をtry_getで見て、あれば
	/// EnemyChaseComponent::MoveSpeedへSpeedMultiplierを掛ける)。
	/// 特定武器専用にせず汎用コンポーネントにしてあるため、将来スロウを付与する武器が
	/// 増えても同じ仕組みを使い回せる(現状はOrbitWeaponSystemのみが付与)。
	/// </summary>
	struct EnemySlowStatusComponent
	{
		/// <summary>移動速度倍率(1.0=通常、0.5=50%減速)</summary>
		float SpeedMultiplier = 1.0f;

		/// <summary>残り効果時間(秒)。0以下でEnemySlowStatusSystemがコンポーネントごと外す</summary>
		float RemainingDuration = 0.0f;
	};
}
