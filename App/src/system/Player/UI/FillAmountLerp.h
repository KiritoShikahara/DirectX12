#pragma once

#include<Utility/Export/Export.h>
#include<algorithm>

namespace ecs
{
	///<summary>
	///Sprite::FillAmountを目標値へ滑らかに追従させるためのコンポーネント。体力ゲージ等、値を即時反映せず徐々に増減させたい対象に付ける
	///</summary>
	struct ENGINE_API FillAmountLerp
	{
		///<summary>
		///目標のFillAmount、0から1
		///</summary>
		float Target = 1.0f;

		///<summary>
		///1秒あたりの変化量。2.0なら0.5秒で0から1
		///</summary>
		float Speed = 2.0f;

		///<summary>
		///目標値を設定する、0から1にクランプ
		///</summary>
		void SetTarget(float t)
		{
			Target = std::clamp(t, 0.0f, 1.0f);
		}
	};
}
