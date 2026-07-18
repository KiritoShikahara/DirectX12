#include "apppch.h"
#include "ProjectileMovementSystem.h"

#include"ProjectileComponent.h"
#include<system/Player/PlayerActionLock.h>

namespace ecs
{
	void ProjectileMovementSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// 必殺技演出中は既存の弾の移動も一時停止させる(Homing Missile/Bone Spear等の
		// 自動発動武器の弾も含むため、Flicker Strike中は止めない。PlayerActionLock.h参照)
		if (ecs::IsPlayerUltimateActive(registry)) return;

		mExpired.clear();

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
					mExpired.push_back(entity);
				}
			});

		// 何にも命中しなかった弾はここで消滅させる（衝突破棄は ProjectileCollisionSystem 側）
		for (entt::entity entity : mExpired)
		{
			registry.destroy(entity);
		}
	}
}
