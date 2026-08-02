#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///EnemyStatusComponent::CurrentHpが0以下になった敵を破棄する。ダメージ計算とライフサイクル管理を分離し、撃破報酬もここで扱う
	///</summary>
	class EnemyDeathSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///撃破で得た合計経験値をプレイヤーへ加算し、閾値到達ならレベルアップを要求する
		///</summary>
		static void AwardExperience(entt::registry& registry, float experience);

		///<summary>
		///撃破数を必殺技ゲージへ加算する。ゲージ満タン中・発動中は加算しない
		///</summary>
		static void AwardUltimateCharge(entt::registry& registry, int killCount);

		///<summary>
		///撃破で得た合計ゴールドへゴールド獲得量強化の倍率をかけ、PlayerSaveDataへ加算し即座に保存する
		///</summary>
		static void AwardGold(float gold);

		///<summary>
		///撃破数をパワーチャージへ加算する。FlickerStrikeWeaponData::MaxChargeで頭打ち
		///</summary>
		static void AwardPowerCharge(entt::registry& registry, int killCount);
	};
}
