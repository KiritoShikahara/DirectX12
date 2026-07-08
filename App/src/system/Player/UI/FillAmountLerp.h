#pragma once

#include<Utility/Export/Export.h>
#include<algorithm>

namespace ecs
{
	/// <summary>
	/// Sprite::FillAmount を目標値へ滑らかに追従させるためのコンポーネント。
	/// 体力ゲージ等、値を即時反映せず徐々に増減させたい対象に付ける。
	/// </summary>
	struct ENGINE_API FillAmountLerp
	{
		/// <summary>目標 FillAmount [0,1]</summary>
		float Target = 1.0f;

		/// <summary>1秒あたりの変化量（0.5f なら 2秒で 0→1）</summary>
		float Speed = 2.0f;

		/// <summary>目標値を設定する（[0,1] にクランプ）</summary>
		void SetTarget(float t)
		{
			Target = std::clamp(t, 0.0f, 1.0f);
		}
	};
}