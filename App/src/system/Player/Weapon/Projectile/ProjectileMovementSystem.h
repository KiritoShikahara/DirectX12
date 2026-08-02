#pragma once

#include<entt/entt.hpp>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    ///<summary>
    ///ProjectileComponentを持つエンティティを直進させる。命中せずLifeTimeを超えたら自動的に破棄する
    ///</summary>
    class ProjectileMovementSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///Updateの一時バッファ、寿命切れの弾。毎回clearして再利用する
        ///</summary>
        std::vector<entt::entity> mExpired;
    };
}
