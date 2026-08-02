#include "apppch.h"
#include "LocomotionAnimationSystem.h"
#include "ActionAnimLockComponent.h"

#include<graphics/Fbx/Resource/FbxResource.h>
#include<system/MoveDirection/MoveDirectionComponent.h>

namespace ecs
{
	void LocomotionAnimationSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// ActionAnimLockComponentの残り時間を減らし、尽きたものは外す
		mExpiredLocks.clear();
		registry.view<ActionAnimLockComponent>().each(
			[&](entt::entity entity, ActionAnimLockComponent& lock)
			{
				lock.RemainingTime -= deltaTime;
				if (lock.RemainingTime <= 0.0f)
				{
					mExpiredLocks.push_back(entity);
				}
			});
		for (entt::entity entity : mExpiredLocks)
		{
			registry.remove<ActionAnimLockComponent>(entity);
		}

		registry.view<FbxComponent, FbxAnimComponent, MoveDirectionComponent>().each(
			[&](entt::entity entity, const FbxComponent& fbx, FbxAnimComponent& anim, const MoveDirectionComponent& moveDir)
			{
				if (fbx.Resource == nullptr) return;

				// ロック中はIdle/Runへ自動で戻さない
				if (registry.all_of<ActionAnimLockComponent>(entity)) return;

				const char* desiredClip = moveDir.IsMoving ? "Run" : "Idle";
				const int desiredIndex = fbx.Resource->FindClipIndex(desiredClip);
				if (desiredIndex < 0) return; // クリップ未登録なら切り替えない、現在の再生を維持

				// 目的クリップが現在と違うときだけCrossFadeする。毎フレーム呼ぶとブレンドが再始動し続けるため
				if (anim.CurrentClipIndex != desiredIndex)
				{
					anim.CrossFade(*fbx.Resource, desiredClip, 0.2f, true);
				}
			});
	}
}
