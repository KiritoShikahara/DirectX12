#include "AnimationSystem.h"
#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/fbx/FbxComponent.h>
#include<ecs/component/fbx/AnimationComponent.h>

namespace graphics
{
	void AnimationSystem::UpdateAnimation(entt::registry& registry, float deltaTime)
	{
		auto view = registry.view<ecs::FbxModel, ecs::AnimationComponent>();

		view.each([&](auto entity, const ecs::FbxModel& fbxModel, ecs::AnimationComponent& animComp)
			{
				if (animComp.IsPlaying && fbxModel.Resource != nullptr)
				{
					animComp.Update(deltaTime, *fbxModel.Resource);
					//animComp.CalcBoneMatrices(*fbxModel.Resource);
				}
			});
	}

}