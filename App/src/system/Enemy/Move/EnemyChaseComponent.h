#pragma once

namespace ecs
{
	/// <summary>
	/// プレイヤーに向かって追従にする敵のパラメーター
	/// </summary>
	struct EnemyChaseComponent
	{
		/// <summary>追尾移動速度</summary>
		float MoveSpeed = 0.0f;

		/// <summary>この距離内は移動処理をしない</summary>
		float StopDistance = 0.0f;

	};
} 