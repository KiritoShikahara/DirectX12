#pragma once

namespace ecs
{
	///<summary>
	///敵の移動速度減速デバフ、スロウの実行時状態。EnemyChaseSystemがtry_getで見てMoveSpeedへSpeedMultiplierを掛ける
	///</summary>
	struct EnemySlowStatusComponent
	{
		///<summary>
		///移動速度倍率。1.0で通常、0.5で50%減速
		///</summary>
		float SpeedMultiplier = 1.0f;

		///<summary>
		///残り効果時間、秒。0以下でEnemySlowStatusSystemがコンポーネントごと外す
		///</summary>
		float RemainingDuration = 0.0f;
	};
}
