#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// 必殺技(Ultimate)ゲージUIの充填率を更新するシステム。
	/// プレイヤーの撃破数(PlayerUltimateComponent::KillCount)を UltimateData::RequiredKillCount で
	/// 割った比率を、PlayerUltimateGaugeTag を持つスプライトの Sprite::FillAmount へ反映する。
	///
	/// 体力バーの PlayerHpBarSystem と処理構造は同一で、比率の算出元が
	/// 「現在HP / 最大HP」か「撃破数 / 必要撃破数」かだけが異なる。
	/// (将来この2つを1つの汎用ゲージシステムへ統合する余地はあるが、現状は既存様式に合わせて並行実装)
	/// </summary>
	class PlayerUltimateGaugeSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
