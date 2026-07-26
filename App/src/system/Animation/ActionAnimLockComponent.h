#pragma once

namespace ecs
{
	/// <summary>
	/// 単発アクションアニメーション(Attack_A等)の再生中、LocomotionAnimationSystemによる
	/// Idle/Runへの自動切り替えを止めておくためのロック。付与している間はLocomotionAnimationSystemが
	/// このエンティティを素通りする。RemainingTimeが尽きたらLocomotionAnimationSystem側で自動的に
	/// 外れ、以後は通常のIdle/Run切り替えへ戻る。
	/// 特定武器専用にせず汎用コンポーネントにしてあるため、将来別の単発アクションを
	/// 追加する場合も同じ仕組みを使い回せる(現状はFlickerStrikeWeaponSystemのみが付与)。
	/// </summary>
	struct ActionAnimLockComponent
	{
		float RemainingTime = 0.0f;
	};
}
