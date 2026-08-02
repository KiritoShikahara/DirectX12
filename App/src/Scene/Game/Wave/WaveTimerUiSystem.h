#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    ///<summary>
    ///WaveComponentの残り時間をMM:SS形式でWaveTimerUiTagの付いたTextComponentへ反映するシステム
    ///</summary>
    class WaveTimerUiSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
    };
}
