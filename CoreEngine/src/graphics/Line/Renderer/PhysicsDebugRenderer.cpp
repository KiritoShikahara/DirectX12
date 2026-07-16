#include "pch.h"
#include "PhysicsDebugRenderer.h"

// Jolt
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Geometry/AABox.h>

// dx12
#include <d3dx12.h>
#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Dx12/Dx12Renderer.h>
#include <graphics/Shader/ShaderManager.h>
#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>

// ecs
#include <ecs/component/rigidbody/RigidbodyComponent.h>
#include <ecs/component/collider/ColliderComponent.h>
#include <ecs/component/transform/TransformComponent.h>
#include <ecs/component/camera/CameraComponent.h>
#include <ecs/component/Debug/DebugWireSphereComponent.h>

// sys
#include <system/Camera/CameraSystem.h>
#include <system/Physics/Manager/PhysicsManager.h>
#include <system/ImGui/ImGuiManager.h>

namespace graphics
{

    bool PhysicsDebugRenderer::Initialize()
    {
        if (mIsInitialized) return true;

        mHeapManager = &graphics::GDescriptorHeapManager::Get();

        mLineVertices.reserve(kMaxVertices);

        // パイプライン作成
        mPipeline = std::make_unique<LinePipeline>();
        if (!mPipeline->Initialize())
        {
            DEBUG_LOG(sys::eLogLevel::Error, "PhysicsDebugRenderer: Failed to create pipeline.");
            return false;
        }

        // カメラ ConstantBuffer 作成
        if (!CreateCameraBuffer())
        {
            DEBUG_LOG(sys::eLogLevel::Error, "PhysicsDebugRenderer: Failed to create camera buffer.");
            return false;
        }

        // 動的頂点バッファ作成（FRAME_COUNT 個のリングとして確保される）
        mVertexBuffer = std::make_unique<VertexBuffer>();
        if (!mVertexBuffer->CreateDynamic(sizeof(WireVertex) * kMaxVertices, sizeof(WireVertex)))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "PhysicsDebugRenderer: Failed to create vertex buffer.");
            return false;
        }

        // ImGui ウィンドウ登録
        sys::ImGuiManager::Get().AddDebugUI([this]() { ImGuiWindow(); }, "Physics");

        mIsInitialized = true;
        DEBUG_LOG(sys::eLogLevel::Log, "PhysicsDebugRenderer: Initialized.");
        return true;
    }

    void PhysicsDebugRenderer::Finalize()
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

    bool PhysicsDebugRenderer::CreateCameraBuffer()
    {
        auto& device = graphics::DX12Device::Get();

        // 自作 ConstantBuffer クラスを使って初期化
        // 内部で 256 バイトアライメント計算と CBV 登録が全自動で行われる
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

    void PhysicsDebugRenderer::ImGuiWindow()
    {
        if (!ImGui::Begin("Physics Debug"))
        {
            ImGui::End();
            return;
        }

        ImGui::Checkbox("Show Colliders", &mEnabled);

        if (mEnabled)
        {
            ImGui::Separator();
            ImGui::Text("Wire Colors (by MotionType)");
            ImGui::ColorEdit4("Dynamic", &mDynamicColor.x);
            ImGui::ColorEdit4("Static", &mStaticColor.x);
            ImGui::ColorEdit4("Kinematic", &mKinematicColor.x);
            ImGui::ColorEdit4("Sensor", &mSensorColor.x);
        }

        ImGui::End();
    }

    // ==============================================================
    //  Begin  ― 前フレームの描画データをクリアする
    // ==============================================================

    void PhysicsDebugRenderer::Begin()
    {
        mLineVertices.clear();
        mDrawVertexCount = 0;
    }

    // ==============================================================
    //  UpdateAndDraw  ― 収集フェーズ
    //
    //  registry / Jolt / CameraSystem から頂点を構築し、
    //  カメラ定数バッファと頂点バッファへ転送するところまでを行う。
    //  コマンドリストへの記録は一切行わない。
    // ==============================================================

    void PhysicsDebugRenderer::UpdateAndDraw(entt::registry& registry)
    {
        if (!mIsInitialized || !mEnabled) return;

        // ── カメラ VP 行列を書き込む ──────────────────────────────────
        auto& cameraSys = sys::CameraSystem::Get();
        if (!cameraSys.HasMainCamera()) return;

        const auto* cam = registry.try_get<ecs::CameraComponent>(
            cameraSys.GetMainCameraEntity());
        if (!cam) return;

        // HLSL は row_major なので転置して渡す
        CameraData camData;
        DirectX::XMStoreFloat4x4(
            &camData.ViewProjection,
            DirectX::XMMatrixTranspose(cam->GetViewProjectionMatrix()));

        // 現在フレームの ConstantBuffer へ転送する
        mCameraBuffer->Update(&camData, sizeof(CameraData));

        // ── Jolt から Shape の三角形を取り出して頂点構築 ─────────────
        auto& bodyInterface = sys::PhysicsManager::Get().GetBodyInterface();

        registry.view<ecs::RigidBodyComponent, ecs::ColliderComponent>().each(
            [&](entt::entity entity,
                const ecs::RigidBodyComponent& rb,
                const ecs::ColliderComponent& /*collider*/)
            {
                if (!rb.IsBodyCreated) return;

                // MotionType で色を決定
                const bool isSensor = registry.all_of<ecs::SensorTagComponent>(entity);
                DirectX::XMFLOAT4 color;
                if (isSensor)
                {
                    color = mSensorColor;
                }
                else
                {
                    switch (rb.MotionType)
                    {
                    case ecs::eMotionType::Dynamic:    color = mDynamicColor;   break;
                    case ecs::eMotionType::Static:     color = mStaticColor;    break;
                    case ecs::eMotionType::Kinematic:  color = mKinematicColor; break;
                    default:                           color = mDynamicColor;   break;
                    }
                }

                // Jolt の Body ワールド行列を取得
                const JPH::RMat44 joltWorld = bodyInterface.GetWorldTransform(rb.BodyID);

                // ── GetTrianglesStart に Jolt のワールド変換を直接渡す ──
                // これで triangle 頂点が最初からワールド空間で返ってくるため、
                // C++ 側での手動の XMMATRIX 変換やオフセット計算が不要になる。
                JPH::ShapeRefC shape = bodyInterface.GetShape(rb.BodyID);
                if (!shape) return;

                JPH::Shape::GetTrianglesContext context;
                shape->GetTrianglesStart(
                    context,
                    JPH::AABox::sBiggest(),
                    joltWorld.GetTranslation(),    // COM のワールド位置
                    joltWorld.GetQuaternion(),     // ワールド回転
                    JPH::Vec3::sReplicate(1.0f));  // スケール

                static constexpr int kBatchSize = 64;
                JPH::Float3 joltVerts[kBatchSize * 3];

                while (true)
                {
                    const int triCount = shape->GetTrianglesNext(
                        context, kBatchSize, joltVerts, nullptr);
                    if (triCount == 0) break;

                    for (int i = 0; i < triCount; ++i)
                    {
                        // 頂点はすでにワールド空間 → そのまま割り当て
                        const DirectX::XMFLOAT3 p0 =
                        { joltVerts[i * 3 + 0].x, joltVerts[i * 3 + 0].y, joltVerts[i * 3 + 0].z };
                        const DirectX::XMFLOAT3 p1 =
                        { joltVerts[i * 3 + 1].x, joltVerts[i * 3 + 1].y, joltVerts[i * 3 + 1].z };
                        const DirectX::XMFLOAT3 p2 =
                        { joltVerts[i * 3 + 2].x, joltVerts[i * 3 + 2].y, joltVerts[i * 3 + 2].z };

                        PushLine(p0, p1, color);
                        PushLine(p1, p2, color);
                        PushLine(p2, p0, color);
                    }
                }
            });

        // ── Jolt に登録されないアドホックな当たり判定(OverlapSphere等)の可視化 ──────
        // DebugWireSphereComponent が付与されたエンティティをワイヤーフレーム球として描画する。
        registry.view<ecs::Transform, ecs::DebugWireSphereComponent>().each(
            [&](const ecs::Transform& tr, const ecs::DebugWireSphereComponent& wire)
            {
                PushWireSphere(tr.GetPosition(), wire.Radius, wire.Color);
            });

        if (mLineVertices.empty()) return;

        // ── 頂点バッファに転送（現在フレームのリソースへ書き込まれる）──
        const size_t vertCount = std::min(mLineVertices.size(), kMaxVertices);
        const size_t uploadSize = sizeof(WireVertex) * vertCount;
        mVertexBuffer->Update(mLineVertices.data(), uploadSize, 0);

        // End() が参照する描画頂点数を確定する
        mDrawVertexCount = static_cast<UINT>(vertCount);
    }

    // ==============================================================
    //  End  ― 記録フェーズ
    //
    //  収集済みデータをコマンドリストへ記録するだけ。
    //  registry / GPU バッファの Update には一切触れない。
    // ==============================================================

    void PhysicsDebugRenderer::End(ID3D12GraphicsCommandList* cmdList)
    {
        if (!mIsInitialized || !mEnabled) return;
        if (cmdList == nullptr) return;
        if (mDrawVertexCount == 0) return;

        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());

        // ConstantBuffer から現在フレームの GPU ハンドルを取得してバインドする
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

    void PhysicsDebugRenderer::PushLine(
        const DirectX::XMFLOAT3& from,
        const DirectX::XMFLOAT3& to,
        const DirectX::XMFLOAT4& color)
    {
        if (mLineVertices.size() + 2 > kMaxVertices) return;
        mLineVertices.push_back({ from, color });
        mLineVertices.push_back({ to,   color });
    }

    // ==============================================================
    //  PushWireSphere
    //  XY/XZ/YZ の3つの円で球を近似する（軽量・実装単純さ優先）。
    // ==============================================================

    void PhysicsDebugRenderer::PushWireSphere(
        const DirectX::XMFLOAT3& center,
        float radius,
        const DirectX::XMFLOAT4& color)
    {
        constexpr int kSegments = 24;

        for (int axis = 0; axis < 3; ++axis) // 0:XY 1:XZ 2:YZ
        {
            DirectX::XMFLOAT3 prev{};
            for (int i = 0; i <= kSegments; ++i)
            {
                const float t = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(kSegments);
                const float c = std::cos(t) * radius;
                const float s = std::sin(t) * radius;

                DirectX::XMFLOAT3 p = center;
                if (axis == 0)      { p.x += c; p.y += s; }
                else if (axis == 1) { p.x += c; p.z += s; }
                else                { p.y += c; p.z += s; }

                if (i > 0) PushLine(prev, p, color);
                prev = p;
            }
        }
    }

} // namespace graphics