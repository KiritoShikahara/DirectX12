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
	/// RotateToMoveComponent 繧呈戟縺､繧ｨ繝ｳ繝・ぅ繝・ぅ縺ｮ蜷代″繧・
	/// RigidBodyComponent::MoveVelocity 縺ｮ譁ｹ蜷代∈蝗櫁ｻ｢縺輔○繧九す繧ｹ繝・Β縲・
	/// 繝励Ξ繧､繝､繝ｼ繝ｻ謨ｵ縲∝・騾壹〒蛻ｩ逕ｨ縺吶ｋ縲・
	/// </summary>
	class ENGINE_API RotateToMoveSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}

