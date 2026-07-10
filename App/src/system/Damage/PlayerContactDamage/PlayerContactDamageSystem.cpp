#include "apppch.h"
#include "PlayerContactDamageSystem.h"

#include<system/Enemy/Attack/EnemyAttackComponent.h>

#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Scene/Game/State/GameState.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<Tag/EntityTag.h>

namespace ecs
{
	void PlayerContactDamageSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto stateView = registry.view<GameStateComponent>();
		if (stateView.begin() == stateView.end()) return;

		auto& gameState = registry.get<GameStateComponent>(*stateView.begin());
		if (gameState.GameState != ::sys::eGameState::InGame) return;

		// 敵全体のクールタイムを進める。
		registry.view<EnemyAttackComponent>().each(
			[&](EnemyAttackComponent& atk)
			{
				if (atk.CooldownTimer > 0.0f)
				{
					atk.CooldownTimer -= deltaTime;
				}
			});

		// プレイヤーの取得
		auto playerView = registry.view<PlayerTag, PlayerStatusComponent, CollisionEnterEvent>();
		if (playerView.size_hint() == 0) return;

		const entt::entity playerEntity = *playerView.begin();
		auto& playerStatus = registry.get<PlayerStatusComponent>(playerEntity);
		const auto& contact = registry.get<CollisionEnterEvent>(playerEntity);

		// 防御計算
		const float defense = playerStatus.Current.Defense;
		const float mitigation = kDefenseHalfPoint / (kDefenseHalfPoint + defense);

		// 接触した敵ごとにダメージ判定
		for (entt::entity other : contact.OtherEntities)
		{
			if (!registry.valid(other)) continue;
			if (!registry.all_of<EnemyTag>(other)) continue;
			
			auto* atk = registry.try_get<EnemyAttackComponent>(other);
			auto* enemyStatus = registry.try_get<EnemyStatusComponent>(other);
			if (!atk || !enemyStatus) continue;

			// クールタイム中はダメージを与えない
			if (atk->CooldownTimer > 0.0f) continue;

			const float damage = enemyStatus->Current.AtkPower * mitigation;
			playerStatus.CurrentHp = std::max(0.0f, playerStatus.CurrentHp - damage);

			atk->CooldownTimer = atk->AttackInterval;
		}

		// 死亡判定からリクエスト申請
		if (playerStatus.CurrentHp <= 0.0f)
		{
			gameState.GameOverRequested = true;
		}
	}

}
