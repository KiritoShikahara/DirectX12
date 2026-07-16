#include "apppch.h"
#include "AreaAttackHazardSystem.h"

#include"AreaAttackHazardComponent.h"
#include<system/Physics/System/PhysicsSystem.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<Tag/EntityTag.h>

namespace ecs
{
	void AreaAttackHazardSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// 必殺技演出中は既存のハザードの継続ダメージも一時停止させる
		if (ecs::IsPlayerUltimateActive(registry)) return;

		std::vector<entt::entity> expired;

		registry.view<AreaAttackHazardComponent, Transform>().each(
			[&](entt::entity entity, AreaAttackHazardComponent& hazard, Transform& transform)
			{
				hazard.RemainingDuration -= deltaTime;
				if (hazard.RemainingDuration <= 0.0f)
				{
					expired.push_back(entity);
					return;
				}

				hazard.TickTimer -= deltaTime;
				if (hazard.TickTimer <= 0.0f)
				{
					ApplyTickDamage(registry, transform.GetPosition(), hazard.Radius, hazard.Damage);
					hazard.TickTimer += hazard.TickInterval;
				}
			});

		// view走査完了後にまとめて破棄する（走査中の破棄はイテレータを不正化しうるため避ける）
		for (entt::entity entity : expired)
		{
			DEBUG_LOG(sys::eLogLevel::Log, "AreaAttackHazardSystem: hazard entity={} expired and destroyed",
				entt::to_integral(entity));
			registry.destroy(entity);
		}
	}

	/// <summary>指定範囲内の敵にダメージを与える（敵タグ以外は無視する）</summary>
	void AreaAttackHazardSystem::ApplyTickDamage(
		entt::registry& registry,
		const DirectX::XMFLOAT3& center,
		float radius,
		float damage)
	{
		std::vector<entt::entity> overlapped;
		::sys::PhysicsSystem::OverlapSphere(registry, center, radius, overlapped);

		for (entt::entity entity : overlapped)
		{
			if (!registry.all_of<ecs::EnemyTag>(entity)) continue;

			auto* status = registry.try_get<ecs::EnemyStatusComponent>(entity);
			if (status == nullptr) continue;

			status->CurrentHp = std::max(0.0f, status->CurrentHp - damage);

			if (const auto* enemyTransform = registry.try_get<ecs::Transform>(entity))
			{
				ecs::combatutil::SpawnDamageNumber(enemyTransform->GetPosition(), damage, false);
			}
		}
	}
}
