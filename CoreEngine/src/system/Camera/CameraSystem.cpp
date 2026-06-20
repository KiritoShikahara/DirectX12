#include"pch.h"
#include "CameraSystem.h"
#include<ecs/component/camera/CameraComponent.h>
#include<ecs/component/transform/TransformComponent.h>

#include<ecs/entity/EntityManager.h>
#include<Editor/Parameter/EditorParameter.h>

namespace sys
{
    bool CameraSystem::Initialize()
    {
#ifdef _DEBUG

        auto& ImGui = ImGuiManager::Get();
        auto& EntityManager = ecs::EntityManager::Get();
        auto& reg = EntityManager.GetRegistry();

        ImGui.AddDebugUI([this, &reg]()
            {
                this->ImGuiUpdate(reg);
            });

#endif // _DEBUG

        return true;
    }

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

        //if (!mHasMainCamera)
        //{
        //    DEBUG_LOG(sys::eLogLevel::Warning,
        //        "CameraSystem: No main camera found.");
        //}
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

    void CameraSystem::ImGuiUpdate(entt::registry& registry)
    {
#ifdef _DEBUG


        if (ImGui::Begin("Camera"))
        {
            // カメラ存在判定
            if (mHasMainCamera && mMainCameraEntity != entt::null)
            {
                // 座標系取得
                if (auto* transform = registry.try_get<ecs::Transform>(mMainCameraEntity))
                {
                    ImGui::Text("Main Camera Active");
                    ImGui::Separator();

                    // カメラの自由操作モードフラグ
                    static bool isFreeCamMode = false;
                    ImGui::Checkbox("Enable Advanced Camera Control", &isFreeCamMode);

                    // 操作可能の時
                    if (isFreeCamMode)
                    {
                        // 操作方法
                        ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "[Control Mode: ACTIVE]");
                        ImGui::Text("- [W/A/S/D] : Move Planar (No Rotation-lock)");
                        ImGui::Text("- [Q/E]     : Move Up / Down (World Y)");
                        ImGui::Text("- [LeftCtrl]: Look around + Move to Facing Direction via [W]");

                        float dt = ImGui::GetIO().DeltaTime;

                        // 左Control時にはUEみたいに向いている方向に移動するようにする。
                        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
                        {
                            ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
                            if ((mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) && !ImGui::IsAnyItemActive())
                            {
                                float yaw = mouseDelta.x * Editor::Camera::RotateSpeed;
                                float pitch = mouseDelta.y * Editor::Camera::RotateSpeed;

                                DirectX::XMVECTOR currentRot = DirectX::XMLoadFloat4(&transform->GetRotation());

                                // 上下回転と左右回転
                                DirectX::XMVECTOR qPitch = DirectX::XMQuaternionRotationAxis(transform->GetRight(), pitch);
                                DirectX::XMVECTOR qYaw = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f), yaw);

                                DirectX::XMVECTOR newRot = DirectX::XMQuaternionMultiply(currentRot, qPitch);
                                newRot = DirectX::XMQuaternionMultiply(newRot, qYaw);
                                transform->SetRotation(DirectX::XMQuaternionNormalize(newRot));
                            }

                            // 向いている方向に移動する。
                            if (ImGui::IsKeyDown(ImGuiKey_W))
                            {
                                DirectX::XMVECTOR forward = transform->GetForward();
                                transform->Translate(DirectX::XMVectorScale(forward, Editor::Camera::MoveSpeed * dt));
                            }
                        }
                        else
                        {
                            DirectX::XMVECTOR right = transform->GetRight();
                            DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
                            DirectX::XMVECTOR forward = DirectX::XMVector3Cross(worldUp, right);

                            DirectX::XMVECTOR movement = DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f);

                            if (ImGui::IsKeyDown(ImGuiKey_W)) movement = DirectX::XMVectorAdd(movement, forward);
                            if (ImGui::IsKeyDown(ImGuiKey_S)) movement = DirectX::XMVectorSubtract(movement, forward);
                            if (ImGui::IsKeyDown(ImGuiKey_D)) movement = DirectX::XMVectorAdd(movement, right);
                            if (ImGui::IsKeyDown(ImGuiKey_A)) movement = DirectX::XMVectorSubtract(movement, right);

                            // Q/E による上下移動 (ワールド座標のY軸方向に固定)
                            if (ImGui::IsKeyDown(ImGuiKey_E)) movement = DirectX::XMVectorAdd(movement, worldUp);
                            if (ImGui::IsKeyDown(ImGuiKey_Q)) movement = DirectX::XMVectorSubtract(movement, worldUp);

                            // 移動量が0でなければ位置を更新
                            if (!DirectX::XMVector3Equal(movement, DirectX::XMVectorZero()))
                            {
                                movement = DirectX::XMVector3Normalize(movement);
                                transform->Translate(DirectX::XMVectorScale(movement, Editor::Camera::MoveSpeed * dt));
                            }
                        }


                        // Escキーでいつでもモードを抜ける
                        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                        {
                            isFreeCamMode = false;
                        }

                    }
                }
            }
            else
            {
                ImGui::Text("No Main Camera Fourd.");
            }
        }

        ImGui::End();
#endif // _DEBUG
    }

}

