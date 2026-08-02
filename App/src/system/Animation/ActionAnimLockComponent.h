#pragma once

namespace ecs
{
	///<summary>
	///単発アクションアニメーション再生中、LocomotionAnimationSystemによるIdle/Runへの自動切り替えを止めるロック。時間切れで自動的に外れる
	///</summary>
	struct ActionAnimLockComponent
	{
		float RemainingTime = 0.0f;
	};
}
