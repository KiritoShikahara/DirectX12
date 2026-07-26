#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    /// <summary>
    /// InGame 荳ｭ縲∵雰縺ｨ繝励Ξ繧､繝､繝ｼ縺ｮ謗･隗ｦ縺ｧ繝励Ξ繧､繝､繝ｼ縺ｸ繝繝｡繝ｼ繧ｸ繧剃ｸ弱∴繧九す繧ｹ繝・Β縲・
    /// 謨ｵ蛟倶ｽ薙＃縺ｨ縺ｮ繧ｯ繝ｼ繝ｫ繧ｿ繧､繝縺ｧ螟壽ｮｵ繝ｻ騾｣邯壹ヲ繝・ヨ繧帝亟縺舌・
    /// </summary>
    class PlayerContactDamageSystem : public IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        static constexpr float kDefenseHalfPoint = 50.0f;
    };
}