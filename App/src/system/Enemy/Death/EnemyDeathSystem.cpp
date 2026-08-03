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
	constexpr const char* kDeathEffectPath = "Assets/Effect/AttackHit.efk";
	constexpr float kDeadSeVolume = 0.2f;
	constexpr float kLevelUpSeVolume = 0.6f;
}

namespace ecs
{
	void EnemyDeathSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		std::vector<entt::entity> dead;
		float totalExperience = 0.0f;
		float totalGold = 0.0f;
		// 必殺技自身の範囲ダメージで倒した敵はゲージ加算対象外のため、通常撃破数と分けて数える
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
			PLAY_SE("Assets/Sound/SE/SE_Dead.aud", false, kDeadSeVolume, false, 1);
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

		// 経験値獲得量強化と今回のプレイ限定のExperienceGainUpパークの両方を乗算する
		data::EnsurePlayerSaveDataLoaded();
		const auto& save = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>().Get();
		float permanentMultiplier = 1.0f;
		if (const auto* upgradeData = DATA_MGR(data::StatUpgradeData).GetById(static_cast<int>(data::eStatUpgradeType::ExperienceGainRate)))
		{
			permanentMultiplier += upgradeData->ValuePerLevel * static_cast<float>(save.ExperienceGainRateLevel);
		}

		level.Experience += experience * level.MulExperienceGain * permanentMultiplier;

		// ボス撃破等で複数レベル分のXPが一度に入ることがあるため、whileループで超過回数分だけレベルアップさせPendingLevelUpCountへ積む
		while (level.Experience >= level.ExperienceToNextLevel)
		{
			level.Experience -= level.ExperienceToNextLevel;
			level.Level += 1;
			level.ExperienceToNextLevel *= level.ExperienceGrowthRate;
			gameState.PendingLevelUpCount += 1;
			PLAY_SE("Assets/Sound/SE/SE_LevelUp.aud", false, kLevelUpSeVolume, false);
		}
	}

	void EnemyDeathSystem::AwardUltimateCharge(entt::registry& registry, int killCount)
	{
		auto playerView = registry.view<PlayerTag, PlayerUltimateComponent>();
		if (playerView.begin() == playerView.end()) return;

		auto& ultimate = registry.get<PlayerUltimateComponent>(*playerView.begin());
		if (ultimate.IsReady || ultimate.IsActive) return; // 満タン中・発動中は撃破しても加算しない

		ultimate.KillCount += killCount;
	}

	void EnemyDeathSystem::AwardGold(float gold)
	{
		data::EnsurePlayerSaveDataLoaded();
		auto& saveMgr = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>();
		auto& save = saveMgr.Get();

		// CooldownRateのBaseと同じ、1.0を基準に加算する方式
		float goldGainMultiplier = 1.0f;
		if (const auto* upgradeData = DATA_MGR(data::StatUpgradeData).GetById(static_cast<int>(data::eStatUpgradeType::GoldGainRate)))
		{
			goldGainMultiplier += upgradeData->ValuePerLevel * static_cast<float>(save.GoldGainRateLevel);
		}

		save.Gold += static_cast<int>(std::lround(gold * goldGainMultiplier));
		saveMgr.Save();
	}

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
