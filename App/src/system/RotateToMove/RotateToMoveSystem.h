#pragma once

#include<entt/entt.hpp>
#include<Utility/Export/Export.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct RotateToMoveComponent; }
namespace ecs { struct Transform; }
namespace ecs { struct RigidBodyComponent; }

namespace ecs
{
	/// <summary>
	/// RotateToMoveComponent を持つエンティティの向きを
	/// RigidBodyComponent::MoveVelocity の方向へ回転させるシステム。
	/// プレイヤー・敵、共通で利用する。
	/// </summary>
	class ENGINE_API RotateToMoveSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}

