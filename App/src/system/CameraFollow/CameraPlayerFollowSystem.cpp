#include "apppch.h"
#include "CameraPlayerFollowSystem.h"
#include <Utility/config/DebugConfig.h>

#include"../Tag/EntityTag.h"
#include"CameraFollowOffsetComponent.h"
#include"CameraOverrideComponent.h"

#include<system/Camera/CameraSystem.h>

namespace ecs
{
    CameraPlayerFollowSystem::CameraPlayerFollowSystem()
    {
#if DEV_TOOL_ENABLED
        ::sys::ImGuiManager::Get().AddDebugUI([this]()
            {
                this->RegisterImgui();
            }, "CameraFollowOffset");
#endif // _DEBUG
    }
    CameraPlayerFollowSystem::~CameraPlayerFollowSystem()
    {
#if DEV_TOOL_ENABLED
        ::sys::ImGuiManager::Get().RemoveDebugUI("CameraFollowOffset");
#endif // _DEBUG
    }

    void CameraPlayerFollowSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
        // PlayerTagを持つ最初のエンティティを探す
        entt::entity playerEntity = entt::null;
        registry.view<ecs::Transform, ecs::PlayerTag>()
            .each([&](entt::entity entity, ecs::Transform&)
                {
                    if (playerEntity == entt::null)
                    {
                        playerEntity = entity;
                    }
                });

        if (playerEntity == entt::null) return;

        const DirectX::XMFLOAT3& playerPos =
            registry.get<ecs::Transform>(playerEntity).GetPosition();

        registry.view<ecs::Transform, ecs::CameraFollowOffsetComponent>()
            .each([&](entt::entity entity,
                ecs::Transform& cameraTransform,
                ecs::CameraFollowOffsetComponent& follow)
                {
                    if (entity == playerEntity) return;

                    // 他のSystemがCameraOverrideComponentで排他的なカメラ位置を要求していればそちらを優先する。Transformの書き込みはこのシステムのみが行う
                    if (const auto* cameraOverride = registry.try_get<ecs::CameraOverrideComponent>(entity))
                    {
                        cameraTransform.SetPosition(cameraOverride->Position);
                        cameraTransform.LookAt(cameraOverride->LookAt);
                        return;
                    }

                    const DirectX::XMFLOAT3& posOffset = follow.Offset;
                    const DirectX::XMFLOAT3& lookOffset = follow.LookAtOffset;

                    cameraTransform.SetPosition(
                        playerPos.x + posOffset.x,
                        playerPos.y + posOffset.y,
                        playerPos.z + posOffset.z);

                    const DirectX::XMFLOAT3 lookAtTarget = {
                        playerPos.x + lookOffset.x,
                        playerPos.y + lookOffset.y,
                        playerPos.z + lookOffset.z
                    };

                    cameraTransform.LookAt(lookAtTarget);
                });
	}

    void CameraPlayerFollowSystem::RegisterImgui()
    {
        if (ImGui::Begin("CameraFollowOffset"))
        {
            auto& manager = ENTITY_MANAGER;
            auto& registry = manager.GetRegistry();

            const entt::entity cameraEntity = ::sys::CameraSystem::Get().GetMainCameraEntity();

            if (cameraEntity != entt::null && registry.valid(cameraEntity))
            {
                if (auto* follow = registry.try_get<ecs::CameraFollowOffsetComponent>(cameraEntity))
                {
                    ImGui::Text("Camera Follow Offset");
                    ImGui::Separator();
                    ImGui::DragFloat3("Position Offset", &follow->Offset.x, 0.1f);
                    ImGui::DragFloat3("LookAt Offset", &follow->LookAtOffset.x, 0.1f);
                }
                else
                {
                    ImGui::Text("CameraFollowOffsetComponent not found.");
                }
            }
            else
            {
                ImGui::Text("No main camera found.");
            }
        }
        ImGui::End();
    }

}

