#pragma once

namespace ecs
{
	/// <summary>
	/// 設置された氷柱1本分のランタイム状態。
	/// AreaAttackWeaponSystem::Fire() が対象の敵ごとに1体ずつ生成し、
	/// AreaAttackHazardSystem が持続時間の管理とダメージの反復適用を行う。
	/// </summary>
	struct AreaAttackHazardComponent
	{
		/// <summary>判定半径(m)</summary>
		float Radius = 5.0f;

		/// <summary>1tickあたりのダメージ</summary>
		float Damage = 8.0f;

		/// <summary>残り持続時間(秒)。0以下で消滅する</summary>
		float RemainingDuration = 5.0f;

		/// <summary>次のダメージ判定までの残り時間(秒)。0以下でダメージを与えてリセットする</summary>
		float TickTimer = 0.0f;

		/// <summary>ダメージ判定の間隔(秒)</summary>
		float TickInterval = 0.5f;
	};
}
