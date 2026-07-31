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

        // パイプライン作成 (PhysicsDebugRenderer と同じ汎用 LinePipeline を再利用)
        mPipeline = std::make_unique<LinePipeline>();
        if (!mPipeline->Initialize())
        {
            DEBUG_LOG(sys::eLogLevel::Error, "LightDebugRenderer: Failed to create pipeline.");
            return false;
        }

        // カメラ ConstantBuffer 作成
        if (!CreateCameraBuffer())
        {
            DEBUG_LOG(sys::eLogLevel::Error, "LightDebugRenderer: Failed to create camera buffer.");
            return false;
        }

        // 動的頂点バッファ作成（FRAME_COUNT 個のリングとして確保される）
        mVertexBuffer = std::make_unique<VertexBuffer>();
        if (!mVertexBuffer->CreateDynamic(sizeof(WireVertex) * kMaxVertices, sizeof(WireVertex)))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "LightDebugRenderer: Failed to create vertex buffer.");
            return false;
        }

        // ImGui ウィンドウ登録
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

    // ==============================================================
    //  CreateCameraBuffer
    // ==============================================================

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

    // ==============================================================
    //  ImGuiWindow
    // ==============================================================

    void LightDebugRenderer::ImGuiWindow()
    {
        if (!ImGui::Begin("Light Gizmo"))
        {
            ImGui::End();
            return;
        }

        // Play中は常に非表示にする(UpdateAndDraw側のガードと対になる)ため、
        // チェックボックス自体もPlay中は無効化して「操作しても効かない」ことを明示する
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

    // ==============================================================
    //  Begin
    // ==============================================================

    void LightDebugRenderer::Begin()
    {
        mLineVertices.clear();
        mDrawVertexCount = 0;
    }

    // ==============================================================
    //  UpdateAndDraw  ― 収集フェーズ
    // ==============================================================

    void LightDebugRenderer::UpdateAndDraw(entt::registry& registry)
    {
        // Play中はゲームプレイの見た目を優先し、編集用のギズモは表示しない
        // (Begin()で毎フレームmDrawVertexCountが0にリセットされるため、ここで収集を
        // スキップするだけでEnd()側も自動的に何も描画しなくなる)
        if (!mIsInitialized || !mEnabled || sys::EditorManager::Get().IsPlaying()) return;

        // ── カメラ VP 行列を書き込む ──────────────────────────────────
        auto& cameraSys = sys::CameraSystem::Get();
        if (!cameraSys.HasMainCamera()) return;

        const auto* cam = registry.try_get<ecs::CameraComponent>(
            cameraSys.GetMainCameraEntity());
        if (!cam) return;

        CameraData camData;
        XMStoreFloat4x4(
            &camData.ViewProjection,
            XMMatrixTranspose(cam->GetViewProjectionMatrix()));
        mCameraBuffer->Update(&camData, sizeof(CameraData));

        // ── DirectionalLight ごとにギズモを構築 ─────────────────────
        // CastShadow の有無に関わらず、方向・位置を常に可視化する。
        registry.view<ecs::DirectionalLightComponent>().each(
            [&](ecs::DirectionalLightComponent& light)
            {
                if (!light.IsActive) return;

                const XMVECTOR dirV = XMVector3Normalize(XMLoadFloat3(&light.Direction));
                XMFLOAT3 dir;
                XMStoreFloat3(&dir, dirV);

                // LightSystem::Update と同じ計算式でライト位置を求める
                // (ライト位置 = 注視点からライト方向の逆向きに ShadowDistance だけ離れた位置)
                const XMVECTOR targetV = XMLoadFloat3(&light.ShadowTarget);
                const XMVECTOR lightPosV = XMVectorSubtract(
                    targetV, XMVectorScale(dirV, light.ShadowDistance));

                XMFLOAT3 lightPos;
                XMStoreFloat3(&lightPos, lightPosV);

                PushDirectionalLightGizmo(lightPos, light.ShadowTarget, dir);
            });

        if (mLineVertices.empty()) return;

        // ── 頂点バッファに転送 ──────────────────────────────────────
        const size_t vertCount = std::min(mLineVertices.size(), kMaxVertices);
        const size_t uploadSize = sizeof(WireVertex) * vertCount;
        mVertexBuffer->Update(mLineVertices.data(), uploadSize, 0);

        mDrawVertexCount = static_cast<UINT>(vertCount);
    }

    // ==============================================================
    //  End  ― 記録フェーズ
    // ==============================================================

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

    // ==============================================================
    //  PushLine
    // ==============================================================

    void LightDebugRenderer::PushLine(
        const XMFLOAT3& from,
        const XMFLOAT3& to,
        const XMFLOAT4& color)
    {
        if (mLineVertices.size() + 2 > kMaxVertices) return;
        mLineVertices.push_back({ from, color });
        mLineVertices.push_back({ to,   color });
    }

    // ==============================================================
    //  PushDirectionalLightGizmo
    // ==============================================================

    void LightDebugRenderer::PushDirectionalLightGizmo(
        const XMFLOAT3& lightPos,
        const XMFLOAT3& target,
        const XMFLOAT3& dir)
    {
        const XMVECTOR dirV = XMLoadFloat3(&dir);
        const XMVECTOR lightPosV = XMLoadFloat3(&lightPos);
        const XMVECTOR targetV = XMLoadFloat3(&target);

        // dir とほぼ平行/反平行な軸を避けて直交基底を作る。
        // (LightSystem::Update の Shadow View 行列と同じ縮退回避ロジック)
        XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        if (std::fabs(XMVectorGetX(XMVector3Dot(dirV, worldUp))) > 0.99f)
            worldUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);

        const XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, dirV));
        const XMVECTOR up2 = XMVector3Cross(dirV, right);

        // ── 軸 (ライト位置 → 注視点) ────────────────────────────────
        PushLine(lightPos, target, mGizmoColor);

        // ── 矢じり (target 手前で軸から開く2本) ─────────────────────
        const float arrowLen = mMarkerSize * 1.5f;
        const float arrowWidth = mMarkerSize * 0.6f;
        const XMVECTOR arrowBaseV = XMVectorSubtract(targetV, XMVectorScale(dirV, arrowLen));

        XMFLOAT3 arrowLeft, arrowRight;
        XMStoreFloat3(&arrowLeft, XMVectorAdd(arrowBaseV, XMVectorScale(right, arrowWidth)));
        XMStoreFloat3(&arrowRight, XMVectorSubtract(arrowBaseV, XMVectorScale(right, arrowWidth)));

        PushLine(arrowLeft, target, mGizmoColor);
        PushLine(arrowRight, target, mGizmoColor);

        // ── ライト位置マーカー (十字) ────────────────────────────────
        XMFLOAT3 rp, rn, up, un;
        XMStoreFloat3(&rp, XMVectorAdd(lightPosV, XMVectorScale(right, mMarkerSize)));
        XMStoreFloat3(&rn, XMVectorSubtract(lightPosV, XMVectorScale(right, mMarkerSize)));
        XMStoreFloat3(&up, XMVectorAdd(lightPosV, XMVectorScale(up2, mMarkerSize)));
        XMStoreFloat3(&un, XMVectorSubtract(lightPosV, XMVectorScale(up2, mMarkerSize)));

        PushLine(rp, rn, mGizmoColor);
        PushLine(up, un, mGizmoColor);
    }

} // namespace graphics
