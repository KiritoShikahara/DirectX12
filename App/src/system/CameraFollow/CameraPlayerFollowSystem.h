#pragma once

#include<entt/entt.hpp>
#include<Utility/Export/Export.h>
#include<ecs/system/manager/IComponentSystem.h>


namespace ecs
{
    /// <summary>
    /// PlayerTag を持つエンティティの方を向き、追従するカメラ制御システム。
    /// カメラの X・Z 座標はプレイヤーに同期し、CameraFollowOffsetComponent::Offset を加算した位置に配置する。
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