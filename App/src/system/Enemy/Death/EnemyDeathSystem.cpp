#include "apppch.h"
#include "EnemyDeathSystem.h"

#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<Scene/Game/State/GameState.h>
#include<Tag/EntityTag.h>
#include<Data/Save/PlayerSaveData.h>

#include<cmath>

namespace ecs
{
	void EnemyDeathSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		std::vector<entt::entity> dead;
		float totalExperience = 0.0f;
		float totalGold = 0.0f;

		registry.view<EnemyTag, EnemyStatusComponent>().each(
			[&](entt::entity entity, const EnemyStatusComponent& status)
			{
				if (status.CurrentHp <= 0.0f)
				{
					dead.push_back(entity);
					totalExperience += status.Base.ExperienceValue;
					totalGold += status.Base.GoldValue;
				}
			});

		if (totalExperience > 0.0f)
		{
			AwardExperience(registry, totalExperience);
		}

		if (totalGold > 0.0f)
		{
			AwardGold(static_cast<int>(std::lround(totalGold)));
		}

		if (!dead.empty())
		{
			AwardUltimateCharge(registry, static_cast<int>(dead.size()));
		}

		for (entt::entity entity : dead)
		{
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

		level.Experience += experience;

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

	/// <summary>撃破で得た合計ゴールドをPlayerSaveData(永続化データ)へ加算し即座に保存する</summary>
	void EnemyDeathSystem::AwardGold(int gold)
	{
		data::EnsurePlayerSaveDataLoaded();
		auto& saveMgr = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>();
		saveMgr.Get().Gold += gold;
		saveMgr.Save();
	}
}
