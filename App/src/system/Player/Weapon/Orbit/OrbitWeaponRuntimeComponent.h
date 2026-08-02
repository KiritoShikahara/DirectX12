#pragma once

#include<vector>
#include<entt/entt.hpp>

namespace ecs
{
	///<summary>
	///SelfDefense型武器専用のランタイム状態。ActiveDuration/CooldownDurationを交互に繰り返すサイクル制のため生成済みオーブ一覧も持つ
	///</summary>
	struct OrbitWeaponRuntimeComponent
	{
		///<summary>
		///周回中のオーブエンティティ、Active中のみ非空でCooldown中は空
		///</summary>
		std::vector<entt::entity> Orbs;

		///<summary>
		///現在Activeフェーズか。既定値falseとPhaseTimer=0により初回Updateで即座にCooldown満了しActiveへ遷移する
		///</summary>
		bool IsActive = false;

		///<summary>
		///現在フェーズの残り時間、秒。0以下でOrbitWeaponSystemがフェーズを切り替える
		///</summary>
		float PhaseTimer = 0.0f;
	};
}
