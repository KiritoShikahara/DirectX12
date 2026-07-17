#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    /// <summary>
    /// EnemyKnockbackComponentを持つ敵へ、その間だけ吹き飛ばし速度を適用し続けるシステム。
    /// RemainingTimeが尽きたらコンポーネントを取り外し、EnemyChaseSystemの通常追従へ
    /// 制御を戻す。EnemyChaseSystem・EnemyKnockbackSystemのどちらも
    /// RigidBodyComponent::MoveVelocityを書き込むため、EnemyChaseSystem側で
    /// 本コンポーネント保持中はスキップするガードを設けて競合を避けている。
    /// </summary>
    class EnemyKnockbackSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
    };
}
