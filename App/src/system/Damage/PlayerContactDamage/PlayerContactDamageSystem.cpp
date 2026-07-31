#include "apppch.h"
#include "PlayerContactDamageSystem.h"

#include<system/Enemy/Attack/EnemyAttackComponent.h>

#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Scene/Game/State/GameState.h>
#include<Scene/Game/Debug/GameDebugSettings.h>
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

		// 敵全体のクールタイムを進める
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
		auto& playerStatus = registry.get<PlayerStatusComponent>(playerEntity);

		// Post-hit invincibility timer (from the shop upgrade) ticks down every frame regardless
		// of contact state, independent of IsInvincible (owned exclusively by Ultimate/Flicker Strike)
		if (playerStatus.PostHitInvincibleTimer > 0.0f)
		{
			playerStatus.PostHitInvincibleTimer = std::max(0.0f, playerStatus.PostHitInvincibleTimer - deltaTime);
		}

		// 衝突が継続している間も毎フレーム検知する必要があるため CollisionStayEvent を使う
		// （CollisionEnterEvent は衝突開始フレームにしか発行されないため、
		// 密着したままだとクールダウンが明けても再ダメージが判定できなくなる）
		const auto* contactPtr = registry.try_get<CollisionStayEvent>(playerEntity);
		if (contactPtr == nullptr) return; // このフレームは何にも触れていない

		if (playerStatus.IsInvincible || playerStatus.PostHitInvincibleTimer > 0.0f) return; // invincible (Ultimate/Flicker Strike, or post-hit invincibility window): skip damage entirely

		// デバッグ用の無敵(GUIまたは--godmodeで有効化)。
		// IsInvincibleはウルト等が終了時にfalseへ戻すため、そちらは流用できない
		if (::debug::GameDebugSettings::Get().IsPlayerInvincible()) return;

		const auto& contact = *contactPtr;

		// 防御力から被ダメージ軽減率を計算
		const float defense = playerStatus.Current.Defense;
		const float mitigation = kDefenseHalfPoint / (kDefenseHalfPoint + defense);

		// 接触している敵ごとにダメージを与える
		bool tookDamage = false; // used below to arm the post-hit invincibility window
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
			tookDamage = true;

			// Damage number shows at the player's own position (isPlayerDamage=true picks the "taken" color)
			if (const auto* playerTransform = registry.try_get<Transform>(playerEntity))
			{
				ecs::combatutil::SpawnDamageNumber(playerTransform->GetPosition(), damage, true);
			}

			atk->CooldownTimer = atk->AttackInterval;
		}

		// Start the post-hit invincibility window if this stat is upgraded (0 duration = feature unused)
		if (tookDamage && playerStatus.Current.PostHitInvincibleDuration > 0.0f)
		{
			playerStatus.PostHitInvincibleTimer = playerStatus.Current.PostHitInvincibleDuration;
		}

		// HPが尽きたらゲームオーバーを要求する
		if (playerStatus.CurrentHp <= 0.0f)
		{
			// 復活パークを取得していれば、1回消費してHP全回復で死亡を取り消す
			if (playerStatus.ReviveCount > 0)
			{
				playerStatus.ReviveCount -= 1;
				playerStatus.CurrentHp = playerStatus.Current.MaxHp;
			}
			else
			{
				gameState.GameOverRequested = true;
			}
		}
	}

}
