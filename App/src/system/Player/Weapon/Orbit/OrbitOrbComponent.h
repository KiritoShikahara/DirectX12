#pragma once

namespace ecs
{
	/// <summary>
	/// 周回するオーブ1個の実行時状態。
	/// オーブ自体はOrbitWeaponSystemが生成・破棄せず、武器が存在する間ずっと存在し続けるため、
	/// SensorEnterEvent(侵入した瞬間のみ発行)ではなくSensorStayEvent(密着中は毎フレーム発行)
	/// を使ったヒットクールダウン方式でダメージを反復する
	/// （PlayerContactDamageSystemの継続ダメージ判定と同じ方針）。
	/// </summary>
	struct OrbitOrbComponent
	{
		/// <summary>現在の周回角度(ラジアン)。初期配置角度(均等割り)+経過時間*OrbitSpeedで更新する</summary>
		float Angle = 0.0f;

		/// <summary>次にダメージを与えられるまでの残り時間(秒)。0以下で反撃可能</summary>
		float HitCooldownTimer = 0.0f;
	};
}
