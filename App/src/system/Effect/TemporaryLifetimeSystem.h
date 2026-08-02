#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    ///<summary>
    ///TemporaryLifetimeComponentを持つエンティティのRemainingTimeを減算し、0以下になったものを破棄する
    ///</summary>
    class TemporaryLifetimeSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
    };
}
