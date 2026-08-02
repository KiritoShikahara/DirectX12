#pragma once

namespace ecs
{
	///<summary>
	///周回するオーブ1個の実行時状態。SensorStayEventを使ったヒットクールダウン方式でダメージを反復する
	///</summary>
	struct OrbitOrbComponent
	{
		///<summary>
		///現在の周回角度、ラジアン。初期配置角度に経過時間×OrbitSpeedを加算して更新する
		///</summary>
		float Angle = 0.0f;

		///<summary>
		///次にダメージを与えられるまでの残り時間、秒。0以下で反撃可能
		///</summary>
		float HitCooldownTimer = 0.0f;
	};
}
