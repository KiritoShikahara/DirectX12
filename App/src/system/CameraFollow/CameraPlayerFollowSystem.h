#pragma once

#include<entt/entt.hpp>
#include<Utility/Export/Export.h>
#include<ecs/system/manager/IComponentSystem.h>


namespace ecs
{
    /// <summary>
    /// PlayerTag 繧呈戟縺､繧ｨ繝ｳ繝・ぅ繝・ぅ縺ｮ譁ｹ繧貞髄縺阪∬ｿｽ蠕薙☆繧九き繝｡繝ｩ蛻ｶ蠕｡繧ｷ繧ｹ繝・Β縲・
    /// 繧ｫ繝｡繝ｩ縺ｮ X繝ｻZ 蠎ｧ讓吶・繝励Ξ繧､繝､繝ｼ縺ｫ蜷梧悄縺励，ameraFollowOffsetComponent::Offset 繧貞刈邂励＠縺滉ｽ咲ｽｮ縺ｫ驟咲ｽｮ縺吶ｋ縲・
    /// </summary>
    class ENGINE_API CameraPlayerFollowSystem : public ecs::IUserSystem
    {
    public:
        CameraPlayerFollowSystem();
        virtual ~CameraPlayerFollowSystem();
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
    private:
        void RegisterImgui();
    };
}