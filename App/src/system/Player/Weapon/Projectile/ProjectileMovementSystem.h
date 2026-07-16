#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    /// <summary>
    /// ProjectileComponent を持つエンティティを直進させる。
    /// 何にも命中しないまま LifeTime を超えたら自動的に破棄する
    /// （命中時の破棄は ProjectileCollisionSystem が行う）。
    /// </summary>
    class ProjectileMovementSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
    };
}
