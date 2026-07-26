#pragma once

#include<vector>
#include<entt/entt.hpp>

namespace ecs
{
	/// <summary>
	/// SelfDefense(周回)型武器（WeaponComponent::Type == SelfDefense）専用のランタイム状態。
	/// 発動トリガーは無いが、常時稼働ではなくActiveDuration(出現)/CooldownDuration(消滅)を
	/// 交互に繰り返すサイクル制のため、生成済みオーブ一覧に加えて現在フェーズの残り時間も持つ
	/// （AreaAttackWeaponRuntimeComponentと同じ方針で、種別ごとに分離する）。
	/// </summary>
	struct OrbitWeaponRuntimeComponent
	{
		/// <summary>周回中のオーブエンティティ（Active中のみ非空。Cooldown中は空）</summary>
		std::vector<entt::entity> Orbs;

		/// <summary>
		/// 現在Activeフェーズ(オーブ出現中)か。既定値falseとPhaseTimer=0の組み合わせにより、
		/// 初回Updateで即座にCooldown満了 → Active開始(オーブ生成)へ遷移する
		/// （初回スポーンのための特別分岐を用意せず、通常のフェーズ切り替えに乗せるため）。
		/// </summary>
		bool IsActive = false;

		/// <summary>現在フェーズの残り時間(秒)。0以下でOrbitWeaponSystemがフェーズを切り替える</summary>
		float PhaseTimer = 0.0f;
	};
}
