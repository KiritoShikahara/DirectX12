#pragma once

#include<entt/entt.hpp>
#include<Utility/Export/Export.h>
#include<ecs/system/manager/IComponentSystem.h>


namespace ecs
{
    ///<summary>
    ///PlayerTagを持つエンティティに追従するカメラ制御システム。カメラのX・Z座標をプレイヤーに同期し、CameraFollowOffsetComponent::Offsetを加算した位置に配置する
    ///</summary>
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
