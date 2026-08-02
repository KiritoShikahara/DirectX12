#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>
#include<vector>

namespace ecs
{
    ///<summary>
    ///ダメージ数値を更新するシステム
    ///</summary>
    class DamageNumberSystem : public ecs::IUserSystem
    {
    public:
        ///<summary>
        ///ダメージ数値を更新
        ///</summary>
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///削除対象のエンティティを保持
        ///</summary>
        std::vector<entt::entity> mExpired;
    };
}