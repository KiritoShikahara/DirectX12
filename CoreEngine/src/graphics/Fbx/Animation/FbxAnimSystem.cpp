#include "pch.h"
#include "FbxAnimSystem.h"

#include <ecs/component/fbx/FbxComponent.h>
#include <ecs/component/fbx/FbxAnimComponent.h>

namespace graphics
{

    void FbxAnimSystem::Update(entt::registry& registry, float deltaTime)
    {
        auto view = registry.view<ecs::FbxComponent, ecs::FbxAnimComponent>();

        view.each([&](auto /*entity*/,
            const ecs::FbxComponent& fbx,
            ecs::FbxAnimComponent& anim)
            {
                if (anim.IsPlaying && fbx.Resource && fbx.Resource->IsLoaded())
                    anim.Update(deltaTime, *fbx.Resource);
            });
    }

} // namespace graphics