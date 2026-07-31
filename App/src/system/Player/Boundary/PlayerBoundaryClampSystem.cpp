#include "apppch.h"
#include "PlayerBoundaryClampSystem.h"

#include<Tag/EntityTag.h>
#include<Scene/Game/Factory/FieldConstants.h>

#include<algorithm>

namespace ecs
{
	void PlayerBoundaryClampSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		constexpr float kLimit = FieldConstants::kPlayableHalfExtent;

		registry.view<PlayerTag, Transform>().each(
			[&](entt::entity entity, Transform& transform)
			{
				const DirectX::XMFLOAT3& pos = transform.GetPosition();

				const float clampedX = std::clamp(pos.x, -kLimit, kLimit);
				const float clampedZ = std::clamp(pos.z, -kLimit, kLimit);

				if (clampedX == pos.x && clampedZ == pos.z) return;

				// SetPosition()だけだとJoltの物理ボディ位置は次の物理ステップで元の(壁の外の)
				// 位置に上書きされてしまうため、TransformDirtyTagでPhysicsSystem::SyncFromTransform
				// に明示的にJolt側へも反映させる(FlickerStrikeWeaponSystem/PlayerUltimateSystemの
				// テレポート処理と同じ作法)
				transform.SetPosition(clampedX, pos.y, clampedZ);
				registry.emplace_or_replace<ecs::TransformDirtyTag>(entity);
			});
	}
}
