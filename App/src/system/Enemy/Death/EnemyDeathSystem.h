#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// EnemyStatusComponent::CurrentHp が0以下になった敵を破棄する。
	/// ダメージを与える各システム(ProjectileCollisionSystem等)は
	/// HPを減らすだけで、生死判定・破棄はここに一本化する
	/// （責務分離：ダメージ計算とライフサイクル管理を分ける）。
	/// 撃破報酬(経験値付与・レベルアップ判定)も「死亡時処理」として同じ場所で扱う。
	/// </summary>
	class EnemyDeathSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>撃破で得た合計経験値をプレイヤーへ加算し、閾値到達ならレベルアップを要求する</summary>
		static void AwardExperience(entt::registry& registry, float experience);

		/// <summary>撃破数を必殺技ゲージへ加算する（ゲージ満タン中・発動中は加算しない）</summary>
		static void AwardUltimateCharge(entt::registry& registry, int killCount);

		/// <summary>撃破で得た合計ゴールドへゴールド獲得量強化(data::eStatUpgradeType::GoldGainRate)
		/// の倍率をかけ、PlayerSaveData(永続化データ)へ加算し即座に保存する</summary>
		static void AwardGold(float gold);

		/// <summary>撃破数をパワーチャージへ加算する（FlickerStrikeWeaponData::MaxChargeで頭打ち）</summary>
		static void AwardPowerCharge(entt::registry& registry, int killCount);
	};
}
