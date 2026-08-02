#include "pch.h"
#include "PhysicsDebugRenderer.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Geometry/AABox.h>

#include <d3dx12.h>
#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Dx12/Dx12Renderer.h>
#include <graphics/Shader/ShaderManager.h>
#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>

#include <ecs/component/rigidbody/RigidbodyComponent.h>
#include <ecs/component/collider/ColliderComponent.h>
#include <ecs/component/transform/TransformComponent.h>
#include <ecs/component/camera/CameraComponent.h>
#include <ecs/component/Debug/DebugWireSphereComponent.h>

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

        mPipeline = std::make_unique<LinePipeline>();
        if (!mPipeline->Initialize())
        {
            DEBUG_LOG(sys::eLogLevel::Error, "PhysicsDebugRenderer: Failed to create pipeline.");
            return false;
        }

        if (!CreateCameraBuffer())
        {
            DEBUG_LOG(sys::eLogLevel::Error, "PhysicsDebugRenderer: Failed to create camera buffer.");
            return false;
        }

        mVertexBuffer = std::make_unique<VertexBuffer>();
        if (!mVertexBuffer->CreateDynamic(sizeof(WireVertex) * kMaxVertices, sizeof(WireVertex)))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "PhysicsDebugRenderer: Failed to create vertex buffer.");
            return false;
        }

#ifdef _DEBUG
        sys::ImGuiManager::Get().AddDebugUI([this]() { ImGuiWindow(); }, "Physics");
#endif

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

    bool PhysicsDebugRenderer::CreateCameraBuffer()
    {
        auto& device = graphics::DX12Device::Get();

        mCameraBuffer = std::make_unique<graphics::ConstantBuffer>();
        if (!mCameraBuffer->Create(device, *mHeapManager, sizeof(CameraData)))
        {
            return false;
        }

        return true;
    }

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

    void PhysicsDebugRenderer::Begin()
    {
        mLineVertices.clear();
        mDrawVertexCount = 0;
    }

    void PhysicsDebugRenderer::UpdateAndDraw(entt::registry& registry)
    {
        if (!mIsInitialized || !mEnabled) return;

        auto& cameraSys = sys::CameraSystem::Get();
        if (!cameraSys.HasMainCamera()) return;

        const auto* cam = registry.try_get<ecs::CameraComponent>(
            cameraSys.GetMainCameraEntity());
        if (!cam) return;

        CameraData camData;
        DirectX::XMStoreFloat4x4(
            &camData.ViewProjection,
            DirectX::XMMatrixTranspose(cam->GetViewProjectionMatrix()));

        mCameraBuffer->Update(&camData, sizeof(CameraData));

        auto& bodyInterface = sys::PhysicsManager::Get().GetBodyInterface();

        registry.view<ecs::RigidBodyComponent, ecs::ColliderComponent>().each(
            [&](entt::entity entity,
                const ecs::RigidBodyComponent& rb,
                const ecs::ColliderComponent& /*collider*/)
            {
                if (!rb.IsBodyCreated) return;

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

                const JPH::RMat44 joltWorld = bodyInterface.GetWorldTransform(rb.BodyID);

                JPH::ShapeRefC shape = bodyInterface.GetShape(rb.BodyID);
                if (!shape) return;

                JPH::Shape::GetTrianglesContext context;
                shape->GetTrianglesStart(
                    context,
                    JPH::AABox::sBiggest(),
                    joltWorld.GetTranslation(),
                    joltWorld.GetQuaternion(),
                    JPH::Vec3::sReplicate(1.0f));

                static constexpr int kBatchSize = 64;
                JPH::Float3 joltVerts[kBatchSize * 3];

                while (true)
                {
                    const int triCount = shape->GetTrianglesNext(
                        context, kBatchSize, joltVerts, nullptr);
                    if (triCount == 0) break;

                    for (int i = 0; i < triCount; ++i)
                    {
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

        registry.view<ecs::Transform, ecs::DebugWireSphereComponent>().each(
            [&](const ecs::Transform& tr, const ecs::DebugWireSphereComponent& wire)
            {
                PushWireSphere(tr.GetPosition(), wire.Radius, wire.Color);
            });

        if (mLineVertices.empty()) return;

        const size_t vertCount = std::min(mLineVertices.size(), kMaxVertices);
        const size_t uploadSize = sizeof(WireVertex) * vertCount;
        mVertexBuffer->Update(mLineVertices.data(), uploadSize, 0);

        mDrawVertexCount = static_cast<UINT>(vertCount);
    }

    void PhysicsDebugRenderer::End(ID3D12GraphicsCommandList* cmdList)
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

    void PhysicsDebugRenderer::PushLine(
        const DirectX::XMFLOAT3& from,
        const DirectX::XMFLOAT3& to,
        const DirectX::XMFLOAT4& color)
    {
        if (mLineVertices.size() + 2 > kMaxVertices) return;
        mLineVertices.push_back({ from, color });
        mLineVertices.push_back({ to,   color });
    }

    void PhysicsDebugRenderer::PushWireSphere(
        const DirectX::XMFLOAT3& center,
        float radius,
        const DirectX::XMFLOAT4& color)
    {
        constexpr int kSegments = 24;

        for (int axis = 0; axis < 3; ++axis)
        {
            DirectX::XMFLOAT3 prev{};
            for (int i = 0; i <= kSegments; ++i)
            {
                const float t = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(kSegments);
                const float c = std::cos(t) * radius;
                const float s = std::sin(t) * radius;

                DirectX::XMFLOAT3 p = center;
                if (axis == 0) { p.x += c; p.y += s; }
                else if (axis == 1) { p.x += c; p.z += s; }
                else { p.y += c; p.z += s; }

                if (i > 0) PushLine(prev, p, color);
                prev = p;
            }
        }
    }

} // namespace graphics