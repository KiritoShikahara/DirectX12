#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// 経験値バー・現在レベル表示のUIを更新するシステム。
	/// プレイヤーの経験値(PlayerLevelComponent::Experience)を次レベルに必要な経験値
	/// (ExperienceToNextLevel)で割った比率を、PlayerExpBarTag を持つスプライトの
	/// Sprite::FillAmount へ反映し、PlayerLevelTextTag のテキストへ現在レベルを表示する。
	///
	/// 体力バーのPlayerHpBarSystem・必殺ゲージのPlayerUltimateGaugeSystemと処理構造は同一で、
	/// 比率の算出元が異なるだけ(既存様式に合わせて並行実装)。
	/// </summary>
	class PlayerExpBarSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
