#include "apppch.h"
#include "ProjectileMovementSystem.h"

#include"ProjectileComponent.h"
#include<system/Player/PlayerActionLock.h>

namespace ecs
{
	void ProjectileMovementSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// 必殺技演出中は既存の弾の移動も一時停止させる
		if (ecs::IsPlayerActionLocked(registry)) return;

		std::vector<entt::entity> expired;

		registry.view<ecs::ProjectileComponent, ecs::RigidBodyComponent>().each(
			[&](entt::entity entity, ecs::ProjectileComponent& projectile, ecs::RigidBodyComponent& rb)
			{
				rb.MoveVelocity.x = projectile.Direction.x * projectile.Speed;
				rb.MoveVelocity.y = 0.0f;
				rb.MoveVelocity.z = projectile.Direction.z * projectile.Speed;
				rb.HasMoveRequest = true;

				projectile.ElapsedTime += deltaTime;
				if (projectile.ElapsedTime >= projectile.LifeTime)
				{
					expired.push_back(entity);
				}
			});

		// 何にも命中しなかった弾はここで消滅させる（衝突破棄は ProjectileCollisionSystem 側）
		for (entt::entity entity : expired)
		{
			registry.destroy(entity);
		}
	}
}
