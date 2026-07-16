#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    /// <summary>
    /// TemporaryLifetimeComponentを持つエンティティのRemainingTimeを減算し、
    /// 0以下になったものを破棄する。デバッグ可視化専用エンティティ等、
    /// EffekseerのautoDeleteに乗らない一時エンティティの後始末を一括して担当する。
    /// </summary>
    class TemporaryLifetimeSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
    };
}
