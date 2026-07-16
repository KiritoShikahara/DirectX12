#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    /// <summary>
    /// WaveComponent の残り時間(ClearTime - ElapsedTime)を MM:SS 形式で
    /// WaveTimerUiTag の付いた TextComponent へ反映するシステム。
    /// </summary>
    class WaveTimerUiSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
    };
}
