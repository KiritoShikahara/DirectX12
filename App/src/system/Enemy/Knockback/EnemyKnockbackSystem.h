#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    ///<summary>
    ///EnemyKnockbackComponentを持つ敵へその間だけ吹き飛ばし速度を適用し続けるシステム。時間切れでコンポーネントを外しEnemyChaseSystemの追従へ戻す
    ///</summary>
    class EnemyKnockbackSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
    };
}
