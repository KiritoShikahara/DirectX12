#include "apppch.h"
#include "PlayerContactDamageSystem.h"

#include<system/Enemy/Attack/EnemyAttackComponent.h>

#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
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

		// �G�S�̂̃N�[���^�C����i�߂�B
		registry.view<EnemyAttackComponent>().each(
			[&](EnemyAttackComponent& atk)
			{
				if (atk.CooldownTimer > 0.0f)
				{
					atk.CooldownTimer -= deltaTime;
				}
			});

		// プレイヤーの取得
		// size_hint() は複数コンポーネントビューでは最小プールのサイズを返すだけで、
		// 実際に全条件を満たすエンティティが存在する保証にはならない
		// （CollisionStayEvent は敵同士の接触でも発行されるため、プレイヤー自身は
		// 何にも触れていないフレームでも size_hint() が非0を返しうる。
		// その状態で *playerView.begin() すると end() を参照してクラッシュする）。
		// そのため PlayerTag + PlayerStatusComponent だけで安全にプレイヤーを取得し、
		// CollisionStayEvent は try_get で有無を確認する。
		auto playerView = registry.view<PlayerTag, PlayerStatusComponent>();
		if (playerView.begin() == playerView.end()) return;

		const entt::entity playerEntity = *playerView.begin();

		// 衝突が継続している間も毎フレーム検知する必要があるため CollisionStayEvent を使う
		// （CollisionEnterEvent は衝突開始フレームにしか発行されないため、
		// 密着したままだとクールダウンが明けても再ダメージが判定できなくなる）
		const auto* contactPtr = registry.try_get<CollisionStayEvent>(playerEntity);
		if (contactPtr == nullptr) return; // このフレームは何にも触れていない

		auto& playerStatus = registry.get<PlayerStatusComponent>(playerEntity);
		if (playerStatus.IsInvincible) return; // invincible (e.g. during Ultimate): skip damage entirely

		const auto& contact = *contactPtr;

		// �h��v�Z
		const float defense = playerStatus.Current.Defense;
		const float mitigation = kDefenseHalfPoint / (kDefenseHalfPoint + defense);

		// �ڐG�����G���ƂɃ_���[�W����
		for (entt::entity other : contact.OtherEntities)
		{
			if (!registry.valid(other)) continue;
			if (!registry.all_of<EnemyTag>(other)) continue;
			
			auto* atk = registry.try_get<EnemyAttackComponent>(other);
			auto* enemyStatus = registry.try_get<EnemyStatusComponent>(other);
			if (!atk || !enemyStatus) continue;

			// �N�[���^�C�����̓_���[�W��^���Ȃ�
			if (atk->CooldownTimer > 0.0f) continue;

			const float damage = enemyStatus->Current.AtkPower * mitigation;
			playerStatus.CurrentHp = std::max(0.0f, playerStatus.CurrentHp - damage);

			// Damage number shows at the player's own position (isPlayerDamage=true picks the "taken" color)
			if (const auto* playerTransform = registry.try_get<Transform>(playerEntity))
			{
				ecs::combatutil::SpawnDamageNumber(playerTransform->GetPosition(), damage, true);
			}

			atk->CooldownTimer = atk->AttackInterval;
		}

		// ���S���肩�烊�N�G�X�g�\��
		if (playerStatus.CurrentHp <= 0.0f)
		{
			gameState.GameOverRequested = true;
		}
	}

}
