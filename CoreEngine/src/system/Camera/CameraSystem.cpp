#include"pch.h"
#include "CameraSystem.h"
#include<ecs/component/camera/CameraComponent.h>
#include<ecs/component/transform/TransformComponent.h>

namespace sys
{
	void CameraSystem::Update(entt::registry& registry)
	{
		const bool needSearch =
			mMainCameraEntity == entt::null ||
			!registry.valid(mMainCameraEntity) ||
			!registry.all_of<ecs::CameraComponent>(mMainCameraEntity);

        if (needSearch)
        {
            SearchMainCamera(registry);
        }
        else
        {
            // IsMainCamera が外部から false にされた場合も再検索
            const auto* cam =
                registry.try_get<ecs::CameraComponent>(mMainCameraEntity);
            if (!cam || !cam->IsMainCamera)
            {
                mMainCameraEntity = entt::null;
                mHasMainCamera = false;
                SearchMainCamera(registry);
            }
        }

        if (!mHasMainCamera) return;

        UpdateMatrices(registry);
	}

    void CameraSystem::SearchMainCamera(entt::registry& registry)
    {
        mMainCameraEntity = entt::null;
        mHasMainCamera = false;

        registry.view<ecs::Transform, ecs::CameraComponent>()
            .each([&](entt::entity entity,
                ecs::Transform&,
                ecs::CameraComponent& cam)
                {
                    if (cam.IsMainCamera && mMainCameraEntity == entt::null)
                    {
                        mMainCameraEntity = entity;
                        mHasMainCamera = true;
                    }
                });

        if (!mHasMainCamera)
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                "CameraSystem: No main camera found.");
        }
    }

    void CameraSystem::UpdateMatrices(entt::registry& registry)
    {
        auto* cam = registry.try_get<ecs::CameraComponent>(mMainCameraEntity);
        auto* tr = registry.try_get<ecs::Transform>(mMainCameraEntity);
        if (!cam || !tr) return;

        cam->UpdateMatrices(*tr);

        using namespace DirectX;
        // VP は HLSL の row_major に合わせて転置
        XMStoreFloat4x4(
            &mShaderData.ViewProjection,
            XMMatrixTranspose(cam->GetViewProjectionMatrix()));
        mShaderData.Position = cam->Position;
    }

    void CameraSystem::SetMainCameraEntity(
        entt::registry& registry, entt::entity entity)
    {
        if (!registry.valid(entity) ||
            !registry.all_of<ecs::CameraComponent>(entity))
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                "CameraSystem::SetMainCameraEntity: Invalid entity.");
            return;
        }

        // 旧メインカメラのフラグを下げる
        if (mMainCameraEntity != entt::null &&
            registry.valid(mMainCameraEntity))
        {
            if (auto* old = registry.try_get<ecs::CameraComponent>(mMainCameraEntity))
                old->IsMainCamera = false;
        }

        registry.get<ecs::CameraComponent>(entity).IsMainCamera = true;
        mMainCameraEntity = entity;
        mHasMainCamera = true;
    }
}

