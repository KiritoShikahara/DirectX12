#pragma once

#include<Utility/Export/Export.h>
#include<DirectXMath.h>

namespace ecs
{
	struct ENGINE_API PlayerController
	{
		/// <summary>最大移動速度 (m/s)</summary>
		float MaxSpeed = 5.0f;

		/// <summary>加速度。小さいほど滑らかに加速する (m/s?)</summary>
		float Acceleration = 30.0f;

		/// <summary>減速度。入力がないときにかかるブレーキ (m/s?)</summary>
		float Deceleration = 20.0f;

		/// <summary>XZ 平面の移動方向（正規化済み・入力なしは 0,0,0）</summary>
		::DirectX::XMFLOAT3 MoveInput = { 0.f, 0.f, 0.f };
	};
}