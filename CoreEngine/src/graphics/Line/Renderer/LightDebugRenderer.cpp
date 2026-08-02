#include "pch.h"
#include "LightDebugRenderer.h"
#include <d3dx12.h>
#include <graphics/Dx12/Dx12Device.h>
#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include <ecs/component/Light/LightComponent.h>
#include <ecs/component/camera/CameraComponent.h>
#include <system/Camera/CameraSystem.h>
#include <system/ImGui/ImGuiManager.h>
#include <system/Editor/EditorManager.h>

using namespace DirectX;

namespace graphics
{
    bool LightDebugRenderer::Initialize()
    {
        if (mIsInitialized) return true;

        mHeapManager = &graphics::GDescriptorHeapManager::Get();
        mLineVertices.reserve(kMaxVertices);

        // パイプライン初期化
        mPipeline = std::make_unique<LinePipeline>();
        if (!mPipeline->Initialize())
        {
            DEBUG_LOG(sys::eLogLevel::Error, "LightDebugRenderer: Failed to create pipeline.");
            return false;
        }

        // カメラ定数バッファ作成
        if (!CreateCameraBuffer())
        {
            DEBUG_LOG(sys::eLogLevel::Error, "LightDebugRenderer: Failed to create camera buffer.");
            return false;
        }

        // 動的頂点バッファ作成
        mVertexBuffer = std::make_unique<VertexBuffer>();
        if (!mVertexBuffer->CreateDynamic(sizeof(WireVertex) * kMaxVertices, sizeof(WireVertex)))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "LightDebugRenderer: Failed to create vertex buffer.");
            return false;
        }

        sys::ImGuiManager::Get().AddDebugUI([this]() { ImGuiWindow(); }, "LightGizmo");

        mIsInitialized = true;
        DEBUG_LOG(sys::eLogLevel::Log, "LightDebugRenderer: Initialized.");
        return true;
    }

    void LightDebugRenderer::Finalize()
    {
        if (!mIsInitialized) return;

        mCameraBuffer.reset();
        mVertexBuffer.reset();
        mPipeline.reset();

        mLineVertices.clear();
        mDrawVertexCount = 0;

        mHeapManager = nullptr;
        mIsInitialized = false;
    }

    bool LightDebugRenderer::CreateCameraBuffer()
    {
        auto& device = graphics::DX12Device::Get();

        mCameraBuffer = std::make_unique<graphics::ConstantBuffer>();
        if (!mCameraBuffer->Create(device, *mHeapManager, sizeof(CameraData)))
        {
            return false;
        }

        return true;
    }

    void LightDebugRenderer::ImGuiWindow()
    {
        if (!ImGui::Begin("Light Gizmo"))
        {
            ImGui::End();
            return;
        }

        const bool isPlaying = sys::EditorManager::Get().IsPlaying();

        ImGui::BeginDisabled(isPlaying);
        ImGui::Checkbox("Show Directional Light Gizmo", &mEnabled);
        ImGui::EndDisabled();

        if (isPlaying)
        {
            ImGui::TextDisabled("(Play中は常に非表示)");
        }
        else if (mEnabled)
        {
            ImGui::ColorEdit4("Gizmo Color", &mGizmoColor.x);
            ImGui::DragFloat("Marker Size", &mMarkerSize, 0.05f, 0.1f, 10.f, "%.2f");
            ImGui::TextDisabled("Line: Light Position -> Shadow Target");
            ImGui::TextDisabled("Cross: Light Position marker");
        }

        ImGui::End();
    }

    void LightDebugRenderer::Begin()
    {
        mLineVertices.clear();
        mDrawVertexCount = 0;
    }

    void LightDebugRenderer::UpdateAndDraw(entt::registry& registry)
    {
        // プレイ中は描画を行わない
        if (!mIsInitialized || !mEnabled || sys::EditorManager::Get().IsPlaying()) return;

        auto& cameraSys = sys::CameraSystem::Get();
        if (!cameraSys.HasMainCamera()) return;

        const auto* cam = registry.try_get<ecs::CameraComponent>(
            cameraSys.GetMainCameraEntity());
        if (!cam) return;

        // カメラ行列の更新
        CameraData camData;
        XMStoreFloat4x4(
            &camData.ViewProjection,
            XMMatrixTranspose(cam->GetViewProjectionMatrix()));
        mCameraBuffer->Update(&camData, sizeof(CameraData));

        // ディレクショナルライトのギズモ生成
        registry.view<ecs::DirectionalLightComponent>().each(
            [&](ecs::DirectionalLightComponent& light)
            {
                if (!light.IsActive) return;

                const XMVECTOR dirV = XMVector3Normalize(XMLoadFloat3(&light.Direction));
                XMFLOAT3 dir;
                XMStoreFloat3(&dir, dirV);

                const XMVECTOR targetV = XMLoadFloat3(&light.ShadowTarget);
                const XMVECTOR lightPosV = XMVectorSubtract(
                    targetV, XMVectorScale(dirV, light.ShadowDistance));

                XMFLOAT3 lightPos;
                XMStoreFloat3(&lightPos, lightPosV);

                PushDirectionalLightGizmo(lightPos, light.ShadowTarget, dir);
            });

        if (mLineVertices.empty()) return;

        // 頂点バッファへ転送
        const size_t vertCount = std::min(mLineVertices.size(), kMaxVertices);
        const size_t uploadSize = sizeof(WireVertex) * vertCount;
        mVertexBuffer->Update(mLineVertices.data(), uploadSize, 0);

        mDrawVertexCount = static_cast<UINT>(vertCount);
    }

    void LightDebugRenderer::End(ID3D12GraphicsCommandList* cmdList)
    {
        if (!mIsInitialized || !mEnabled) return;
        if (cmdList == nullptr) return;
        if (mDrawVertexCount == 0) return;

        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());
        cmdList->SetGraphicsRootDescriptorTable(
            LinePipeline::SLOT_CAMERA_BUFFER,
            mCameraBuffer->GetGpuHandle());

        cmdList->SetPipelineState(mPipeline->GetPipelineState());
        cmdList->IASetPrimitiveTopology(mPipeline->GetTopology());

        mVertexBuffer->Set(cmdList, 0);
        cmdList->DrawInstanced(mDrawVertexCount, 1, 0, 0);
    }

    void LightDebugRenderer::PushLine(
        const XMFLOAT3& from,
        const XMFLOAT3& to,
        const XMFLOAT4& color)
    {
        if (mLineVertices.size() + 2 > kMaxVertices) return;
        mLineVertices.push_back({ from, color });
        mLineVertices.push_back({ to,   color });
    }

    void LightDebugRenderer::PushDirectionalLightGizmo(
        const XMFLOAT3& lightPos,
        const XMFLOAT3& target,
        const XMFLOAT3& dir)
    {
        const XMVECTOR dirV = XMLoadFloat3(&dir);
        const XMVECTOR lightPosV = XMLoadFloat3(&lightPos);
        const XMVECTOR targetV = XMLoadFloat3(&target);

        // 垂直ベクトルの縮退回避
        XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        if (std::fabs(XMVectorGetX(XMVector3Dot(dirV, worldUp))) > 0.99f)
            worldUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);

        const XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, dirV));
        const XMVECTOR up2 = XMVector3Cross(dirV, right);

        // ライト位置からターゲットへのライン
        PushLine(lightPos, target, mGizmoColor);

        // 矢じりの描画
        const float arrowLen = mMarkerSize * 1.5f;
        const float arrowWidth = mMarkerSize * 0.6f;
        const XMVECTOR arrowBaseV = XMVectorSubtract(targetV, XMVectorScale(dirV, arrowLen));

        XMFLOAT3 arrowLeft, arrowRight;
        XMStoreFloat3(&arrowLeft, XMVectorAdd(arrowBaseV, XMVectorScale(right, arrowWidth)));
        XMStoreFloat3(&arrowRight, XMVectorSubtract(arrowBaseV, XMVectorScale(right, arrowWidth)));

        PushLine(arrowLeft, target, mGizmoColor);
        PushLine(arrowRight, target, mGizmoColor);

        // ライト位置の十字マーカー描画
        XMFLOAT3 rp, rn, up, un;
        XMStoreFloat3(&rp, XMVectorAdd(lightPosV, XMVectorScale(right, mMarkerSize)));
        XMStoreFloat3(&rn, XMVectorSubtract(lightPosV, XMVectorScale(right, mMarkerSize)));
        XMStoreFloat3(&up, XMVectorAdd(lightPosV, XMVectorScale(up2, mMarkerSize)));
        XMStoreFloat3(&un, XMVectorSubtract(lightPosV, XMVectorScale(up2, mMarkerSize)));

        PushLine(rp, rn, mGizmoColor);
        PushLine(up, un, mGizmoColor);
    }

} // namespace graphics