#include "pch.h"
#include "ModelAnimationSystem.h"

#include<ecs/component/model/ModelComponent.h>
#include<ecs/component/model/ModelAnimComponent.h>

namespace graphics
{
	void ModelAnimationSystem::Update(entt::registry& registry, float deltaTime)
	{
        auto view = registry.view<ecs::Model, ecs::ModelAnimComponent>();

        view.each([&](auto /*entity*/,
            const ecs::Model& model,
            ecs::ModelAnimComponent& anim)
            {
                if (anim.IsPlaying && model.Resource != nullptr)
                    anim.Update(deltaTime, *model.Resource);
            });
	}
}

