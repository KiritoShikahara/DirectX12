#include"pch.h"
#include "CameraSystem.h"
#include<ecs/component/camera/CameraComponent.h>
#include<ecs/component/transform/TransformComponent.h>

#include<ecs/entity/EntityManager.h>
#include<Editor/Parameter/EditorParameter.h>
#include<system/Window/Window.h>
#include<system/Editor/EditorManager.h>

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
            },"Camera");

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

                    // Editモード中(Play中でない)は常にフリーカメラを有効にする。
                    // UEのエディタカメラと同様、チェックボックスの手動切り替えは不要。
                    const bool isFreeCamMode = sys::EditorManager::Get().IsEditing();

                    // 操作可能の時
                    if (isFreeCamMode)
                    {
                        // 操作方法
                        ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "[Edit Mode: Free Camera ACTIVE]");
                        ImGui::Text("- [W/A/S/D] : Move");
                        ImGui::Text("- [Q/E]     : Move Up / Down (World Y)");
                        ImGui::Text("- [Right Mouse Button + Move] : Look around");

                        const float dt = ImGui::GetIO().DeltaTime;

                        // 左クリックは EditorSystem のオブジェクト選択/ドラッグに使うため、
                        // カメラの視点回転は右クリック押下中のみ有効にする(UE/Unity準拠)。
                        const bool isLooking =
                            ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsAnyItemActive();

                        if (isLooking)
                        {
                            const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
                            if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)
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
                        }

                        // 移動: 右クリック中(視点操作中)はカメラの向きに追従して飛行、
                        //       それ以外は水平面(ピッチ無視)でのパン移動にする。
                        const DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
                        const DirectX::XMVECTOR right = transform->GetRight();
                        const DirectX::XMVECTOR forward = isLooking
                            ? transform->GetForward()
                            : DirectX::XMVector3Cross(worldUp, right);

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
                    else
                    {
                        ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), "[Play Mode: Free Camera disabled]");
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

    Ray CameraSystem::ScreenPointToRay(entt::registry& registry, const DirectX::XMFLOAT2& screenPos) const
    {
        using namespace DirectX;

        Ray ray;

        if (!mHasMainCamera || !registry.valid(mMainCameraEntity))
        {
            return ray;
        }

        const auto* cam = registry.try_get<ecs::CameraComponent>(mMainCameraEntity);
        const auto* tr = registry.try_get<ecs::Transform>(mMainCameraEntity);
        if (!cam || !tr)
        {
            return ray;
        }

        const float viewportWidth = static_cast<float>(::sys::Window::Get().GetVirtualWidth());
        const float viewportHeight = static_cast<float>(::sys::Window::Get().GetVirtualHeight());

        const XMMATRIX view = cam->GetViewMatrix();
        const XMMATRIX proj = cam->GetProjectionMatrix();
        const XMMATRIX world = XMMatrixIdentity();

        const XMVECTOR nearPoint = XMVector3Unproject(
            XMVectorSet(screenPos.x, screenPos.y, 0.f, 0.f),
            0.f, 0.f, viewportWidth, viewportHeight, 0.f, 1.f,
            proj, view, world);

        const XMVECTOR farPoint = XMVector3Unproject(
            XMVectorSet(screenPos.x, screenPos.y, 1.f, 0.f),
            0.f, 0.f, viewportWidth, viewportHeight, 0.f, 1.f,
            proj, view, world);

        XMVECTOR direction = XMVectorSubtract(farPoint, nearPoint);
        if (XMVectorGetX(XMVector3LengthSq(direction)) < 1e-8f)
        {
            return ray;
        }
        direction = XMVector3Normalize(direction);

        XMStoreFloat3(&ray.Origin, XMLoadFloat3(&tr->GetPosition()));
        XMStoreFloat3(&ray.Direction, direction);

        return ray;
    }

    DirectX::XMFLOAT3 CameraSystem::ScreenPointToWorld(entt::registry& registry, const DirectX::XMFLOAT2& screenPos, float distance) const
    {
        using namespace DirectX;

        const Ray ray = ScreenPointToRay(registry, screenPos);

        const XMVECTOR origin = XMLoadFloat3(&ray.Origin);
        const XMVECTOR dir = XMLoadFloat3(&ray.Direction);

        XMFLOAT3 result;
        XMStoreFloat3(&result, XMVectorAdd(origin, XMVectorScale(dir, distance)));
        return result;
    }

    bool CameraSystem::ScreenPointToWorldOnPlaneY(entt::registry& registry, const DirectX::XMFLOAT2& screenPos, float planeY, DirectX::XMFLOAT3& outWorldPos) const
    {
        const Ray ray = ScreenPointToRay(registry, screenPos);

        // 方向のY成分が0に近い = 平面と平行で交差しない
        if (fabsf(ray.Direction.y) < 1e-6f)
        {
            return false;
        }

        const float t = (planeY - ray.Origin.y) / ray.Direction.y;
        if (t < 0.f)
        {
            // 平面がカメラの後方にある
            return false;
        }

        outWorldPos.x = ray.Origin.x + ray.Direction.x * t;
        outWorldPos.y = planeY;
        outWorldPos.z = ray.Origin.z + ray.Direction.z * t;

        return true;
    }

}

