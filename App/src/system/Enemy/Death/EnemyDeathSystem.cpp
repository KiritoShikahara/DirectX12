#include "apppch.h"
#include "EnemyDeathSystem.h"

#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<system/Player/PowerCharge/PlayerPowerChargeComponent.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<Scene/Game/State/GameState.h>
#include<Tag/EntityTag.h>
#include<Data/Save/PlayerSaveData.h>
#include<Data/StatUpgrade/StatUpgradeData.h>

#include<cmath>
#include<algorithm>

namespace
{
	// 敵撃破時の消滅演出。毎撃破ごとに発生する高頻度イベントのため小さく軽量なものを使う
	// (PlayOneShotCombinedの同時再生数/パーティクル数上限で暴走はしない)
	constexpr const char* kDeathEffectPath = "Assets/Effect/AttackHit.efk";
}

namespace ecs
{
	void EnemyDeathSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		std::vector<entt::entity> dead;
		float totalExperience = 0.0f;
		float totalGold = 0.0f;
		// 必殺技自身の範囲ダメージで倒した敵は、必殺技ゲージへ加算しない対象
		// (EnemyStatusComponent::DamagedByUltimate参照)のため、通常撃破数と分けて数える
		int ultimateChargeableKillCount = 0;

		registry.view<EnemyTag, EnemyStatusComponent>().each(
			[&](entt::entity entity, const EnemyStatusComponent& status)
			{
				if (status.CurrentHp <= 0.0f)
				{
					dead.push_back(entity);
					totalExperience += status.Base.ExperienceValue;
					totalGold += status.Base.GoldValue;
					if (!status.DamagedByUltimate)
					{
						ultimateChargeableKillCount += 1;
					}
				}
			});

		if (totalExperience > 0.0f)
		{
			AwardExperience(registry, totalExperience);
		}

		if (totalGold > 0.0f)
		{
			AwardGold(totalGold);
		}

		if (!dead.empty())
		{
			if (ultimateChargeableKillCount > 0)
			{
				AwardUltimateCharge(registry, ultimateChargeableKillCount);
			}
			AwardPowerCharge(registry, static_cast<int>(dead.size()));
		}

		for (entt::entity entity : dead)
		{
			if (const auto* transform = registry.try_get<Transform>(entity))
			{
				ecs::effectutil::PlayOneShotCombined(kDeathEffectPath, transform->GetPosition(), 1.0f);
			}
			registry.destroy(entity);
		}
	}

	void EnemyDeathSystem::AwardExperience(entt::registry& registry, float experience)
	{
		auto playerView = registry.view<PlayerTag, PlayerLevelComponent>();
		if (playerView.begin() == playerView.end()) return;

		auto stateView = registry.view<GameStateComponent>();
		if (stateView.begin() == stateView.end()) return;

		const entt::entity playerEntity = *playerView.begin();
		auto& level = registry.get<PlayerLevelComponent>(playerEntity);
		auto& gameState = registry.get<GameStateComponent>(*stateView.begin());

		// 経験値獲得量強化(data::eStatUpgradeType::ExperienceGainRate、永続)と
		// ExperienceGainUpパーク(level.MulExperienceGain、今回のプレイのみ)の両方を乗算する
		data::EnsurePlayerSaveDataLoaded();
		const auto& save = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>().Get();
		float permanentMultiplier = 1.0f;
		if (const auto* upgradeData = DATA_MGR(data::StatUpgradeData).GetById(static_cast<int>(data::eStatUpgradeType::ExperienceGainRate)))
		{
			permanentMultiplier += upgradeData->ValuePerLevel * static_cast<float>(save.ExperienceGainRateLevel);
		}

		level.Experience += experience * level.MulExperienceGain * permanentMultiplier;

		// ボース撃破・必殺技の全体ダメージ等で複数レベル分のXPが一度に入ることがあるため、
		// whileループで超過した回数分だけレベルアップさせる。GameStateComponent::
		// PendingLevelUpCountへその回数分を積み、パーク選択もレベルアップ回数分だけ
		// 連続で提示する(GameStateSystem参照)。
		while (level.Experience >= level.ExperienceToNextLevel)
		{
			level.Experience -= level.ExperienceToNextLevel;
			level.Level += 1;
			level.ExperienceToNextLevel *= level.ExperienceGrowthRate;
			gameState.PendingLevelUpCount += 1;
		}
	}

	/// <summary>撃破数を必殺技ゲージへ加算する（ゲージ満タン中・発動中は加算しない）</summary>
	void EnemyDeathSystem::AwardUltimateCharge(entt::registry& registry, int killCount)
	{
		auto playerView = registry.view<PlayerTag, PlayerUltimateComponent>();
		if (playerView.begin() == playerView.end()) return;

		auto& ultimate = registry.get<PlayerUltimateComponent>(*playerView.begin());
		if (ultimate.IsReady || ultimate.IsActive) return; // 満タン中・発動中は撃破しても加算しない

		ultimate.KillCount += killCount;
	}

	/// <summary>撃破で得た合計ゴールドへゴールド獲得量強化(data::eStatUpgradeType::GoldGainRate)
	/// の倍率をかけ、PlayerSaveData(永続化データ)へ加算し即座に保存する</summary>
	void EnemyDeathSystem::AwardGold(float gold)
	{
		data::EnsurePlayerSaveDataLoaded();
		auto& saveMgr = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>();
		auto& save = saveMgr.Get();

		// CooldownRateのBaseと同じ「1.0を基準に加算する」方式(ValuePerLevel×レベル)
		float goldGainMultiplier = 1.0f;
		if (const auto* upgradeData = DATA_MGR(data::StatUpgradeData).GetById(static_cast<int>(data::eStatUpgradeType::GoldGainRate)))
		{
			goldGainMultiplier += upgradeData->ValuePerLevel * static_cast<float>(save.GoldGainRateLevel);
		}

		save.Gold += static_cast<int>(std::lround(gold * goldGainMultiplier));
		saveMgr.Save();
	}

	/// <summary>撃破数をパワーチャージへ加算する（ComputeMaxPowerCharge=プレイヤーレベルに
	/// 応じて増加する上限で頭打ち）</summary>
	void EnemyDeathSystem::AwardPowerCharge(entt::registry& registry, int killCount)
	{
		auto playerView = registry.view<PlayerTag, PlayerPowerChargeComponent>();
		if (playerView.begin() == playerView.end()) return;

		const entt::entity playerEntity = *playerView.begin();
		const int maxCharge = ComputeMaxPowerCharge(registry, playerEntity);

		auto& charge = registry.get<PlayerPowerChargeComponent>(playerEntity);
		charge.Count = std::min(maxCharge, charge.Count + killCount);
	}
}
