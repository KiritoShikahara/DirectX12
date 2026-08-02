#include "apppch.h"
#include "AreaAttackHazardSystem.h"

#include"AreaAttackHazardComponent.h"
#include<system/Physics/System/PhysicsSystem.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/PlayerActionLock.h>

namespace ecs
{
	void AreaAttackHazardSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// 必殺技演出中は既存のハザードの継続ダメージも一時停止させる
		if (ecs::IsPlayerActionLocked(registry)) return;

		mExpired.clear();

		registry.view<AreaAttackHazardComponent, Transform>().each(
			[&](entt::entity entity, AreaAttackHazardComponent& hazard, Transform& transform)
			{
				hazard.RemainingDuration -= deltaTime;
				if (hazard.RemainingDuration <= 0.0f)
				{
					mExpired.push_back(entity);
					return;
				}

				hazard.TickTimer -= deltaTime;
				if (hazard.TickTimer <= 0.0f)
				{
					ApplyTickDamage(registry, transform.GetPosition(), hazard.Radius, hazard.Damage);
					hazard.TickTimer += hazard.TickInterval;
				}
			});

		// view走査完了後にまとめて破棄する、走査中の破棄はイテレータを不正化しうるため避ける
		for (entt::entity entity : mExpired)
		{
			DEBUG_LOG(sys::eLogLevel::Log, "AreaAttackHazardSystem: hazard entity={} expired and destroyed",
				entt::to_integral(entity));
			registry.destroy(entity);
		}
	}

	void AreaAttackHazardSystem::ApplyTickDamage(
		entt::registry& registry,
		const DirectX::XMFLOAT3& center,
		float radius,
		float damage)
	{
		mOverlapped.clear();
		::sys::PhysicsSystem::OverlapSphere(registry, center, radius, mOverlapped);

		for (entt::entity entity : mOverlapped)
		{
			ecs::combatutil::ApplyDamageToEnemy(registry, entity, damage);
		}
	}
}
