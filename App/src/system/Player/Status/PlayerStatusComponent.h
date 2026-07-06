#pragma once

namespace ecs
{

	struct BaseStatus
	{
		// 最大体力
		float MaxHp = 1;
		// 移動速度
		float MoveSpeed = 1;
		// 攻撃力
		float AtkPower = 1;
		// クールダウンの減少倍率
		float CooldownRate = 1.0f;
	};

	struct CurrentStatus
	{
		float MaxHp = 0;
		float MoveSpeed = 0;
		float AtkPower = 0;
		float CooldownRate = 1.0;

	};

	/// <summary>
	/// ステータスコンポーネント
	/// </summary>
	struct PlayerStatusComponent
	{
		BaseStatus Base;
		CurrentStatus Current;

		float CurrHp;
	};


}