#pragma once

namespace ecs
{
	///<summary>
	///プレイヤーへ向かって追従移動する敵のパラメーター
	///</summary>
	struct EnemyChaseComponent
	{
		///<summary>
		///追従移動速度
		///</summary>
		float MoveSpeed = 0.0f;

		///<summary>
		///この距離以内は移動処理をしない
		///</summary>
		float StopDistance = 0.0f;

	};
}
