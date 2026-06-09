#include "pch.h"
#include "FbxRenderer.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Texture/Texture.h>
#include <graphics/Shader/ShaderManager.h>
#include <graphics/Fbx/Resource/FbxResource.h>
#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include <graphics/Texture/TextureManager.h>

#include <ecs/component/transform/TransformComponent.h>
#include <ecs/component/Fbx/FbxComponent.h>
#include <ecs/component/Fbx/FbxAnimComponent.h>
#include <system/Camera/CameraSystem.h>

using namespace DirectX;

namespace graphics
{

    bool FbxRenderer::Initialize(
        DX12Device& device,
        GDescriptorHeapManager& heapManager,
        ShaderManager& shaderManager)
    {
        mPipeline = std::make_unique<FbxPipeline>();
        if (!mPipeline->Create(device, shaderManager))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create pipeline.");
            return false;
        }

        mInstanceBuffer = std::make_unique<StructuredBuffer>();
        if (!mInstanceBuffer->Create(sizeof(FbxInstanceData), MAX_FBX_INSTANCES))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create instance buffer.");
            return false;
        }

        mBoneBuffer = std::make_unique<StructuredBuffer>();
        if (!mBoneBuffer->Create(sizeof(XMFLOAT4X4), MAX_TOTAL_BONES))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create bone buffer.");
            return false;
        }

        mSceneBuffer = std::make_unique<StructuredBuffer>();
        if (!mSceneBuffer->Create(sizeof(FbxSceneData), 1))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create scene buffer.");
            return false;
        }

        mLightBuffer = std::make_unique<StructuredBuffer>();
        if (!mLightBuffer->Create(sizeof(LightData), MAX_LIGHTS))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create light buffer.");
            return false;
        }

        auto& texMgr = TextureManager::Get();
        mDefaultWhiteTexture = texMgr.GetOrLoad(ASSET_PATH("/Engine/Assets/Texture/White.dds").string());
        mDefaultNormalTexture = texMgr.GetOrLoad(ASSET_PATH("/Engine/Assets/Texture/Normal.dds").string());
        mDefaultBlackTexture = texMgr.GetOrLoad(ASSET_PATH("/Engine/Assets/Texture/Black.dds").string());

        mHeapManager = &heapManager;

        mInstanceData.reserve(MAX_FBX_INSTANCES);
        mBoneData.reserve(MAX_TOTAL_BONES);
        mDrawCalls.reserve(MAX_FBX_INSTANCES * 4);

        DEBUG_LOG(sys::eLogLevel::Log, "FbxRenderer: Initialized successfully.");
        return true;
    }

    void FbxRenderer::Begin()
    {
        mInstanceData.clear();
        mBoneData.clear();
        mDrawCalls.clear();
    }

    void FbxRenderer::UpdateAndDraw(entt::registry& registry)
    {
        auto& camSystem = sys::CameraSystem::Get();
        if (!camSystem.HasMainCamera()) return;

        const auto& cam = camSystem.GetShaderData();
        FbxSceneData scene = {};
        std::memcpy(&scene.ViewProjection, &cam.ViewProjection, sizeof(XMFLOAT4X4));
        scene.CameraPosition = cam.Position;
        scene.LightCount = static_cast<uint32_t>(mLightData.size());
        mSceneBuffer->Update(&scene, sizeof(FbxSceneData));

        if (!mLightData.empty())
        {
            mLightBuffer->Update(
                mLightData.data(),
                sizeof(LightData) * mLightData.size());
        }

        struct RenderItem
        {
            const ecs::Transform* transform;
            const ecs::FbxComponent* fbx;
            ecs::FbxAnimComponent* anim;
        };

        std::vector<RenderItem> items;
        auto view = registry.view<ecs::Transform, ecs::FbxComponent>();
        items.reserve(view.size_hint());

        view.each([&](auto entity, ecs::Transform& tr, ecs::FbxComponent& fbxComp)
            {
                if (!fbxComp.IsVisible || !fbxComp.Resource || !fbxComp.Resource->IsLoaded()) return;
                ecs::FbxAnimComponent* anim = registry.try_get<ecs::FbxAnimComponent>(entity);
                items.push_back({ &tr, &fbxComp, anim });
            });

        for (auto& item : items)
        {
            const bool hasAnimation = (item.anim && item.fbx->Resource->HasSkinning());

            if (hasAnimation)
            {
                item.anim->CalcBoneMatrices(*item.fbx->Resource);
            }

            // ─── 【重要】ここを修正 ───
            // アニメーションがある場合は、モーションデータの位置を100%信頼するためピボットを (0,0,0) にする。
            // アニメーションがない（静的）場合のみ、AABBからの底面ピボットを適用する。
            XMFLOAT3 pivot = { 0.f, 0.f, 0.f };
            if (!hasAnimation)
            {
                pivot = item.fbx->AutoPivot
                    ? item.fbx->Resource->GetBottomCenterPivot()
                    : item.fbx->PivotOffset;
            }

            XMMATRIX world = item.transform->GetWorldMatrix();

            // 静的モデルの時だけ、このピボット合成が走る
            if (pivot.x != 0.f || pivot.y != 0.f || pivot.z != 0.f)
            {
                world = XMMatrixTranslation(pivot.x, pivot.y, pivot.z) * world;
            }

            XMFLOAT4X4 worldF;
            XMStoreFloat4x4(&worldF, XMMatrixTranspose(world));

            const std::vector<XMFLOAT4X4>* bonePtr =
                (hasAnimation && !item.anim->BoneMatrices.empty())
                ? &item.anim->BoneMatrices
                : nullptr;

            Submit(*item.fbx->Resource, worldF, bonePtr, item.fbx->CustomColor);
        }
    }

    void FbxRenderer::Submit(
        const FbxResource& resource,
        const XMFLOAT4X4& world,
        const std::vector<XMFLOAT4X4>* boneMatrices,
        const XMFLOAT4& customColor)
    {
        const uint32_t boneOffset = static_cast<uint32_t>(mBoneData.size());
        uint32_t       boneCount = 0u;

        if (boneMatrices && !boneMatrices->empty() && resource.HasSkinning())
        {
            boneCount = static_cast<uint32_t>(boneMatrices->size());
            if (boneOffset + boneCount <= MAX_TOTAL_BONES)
            {
                mBoneData.insert(mBoneData.end(), boneMatrices->begin(), boneMatrices->end());
            }
            else
            {
                DEBUG_LOG(sys::eLogLevel::Warning, "FbxRenderer: BoneBuffer overflow. Skinning skipped.");
                boneCount = 0u;
            }
        }

        const auto& sections = resource.GetSections();
        for (uint32_t si = 0; si < static_cast<uint32_t>(sections.size()); ++si)
        {
            if (mInstanceData.size() >= MAX_FBX_INSTANCES)
            {
                DEBUG_LOG(sys::eLogLevel::Warning, "FbxRenderer: InstanceBuffer overflow. Draw skipped.");
                break;
            }

            const FbxSection& sec = sections[si];

            FbxInstanceData inst = {};
            inst.World = world;
            inst.BaseColorFactor = sec.BaseColorFactor;
            inst.MetallicFactor = sec.MetallicFactor;
            inst.RoughnessFactor = sec.RoughnessFactor;
            inst.EmissiveFactor = sec.EmissiveFactor;
            inst.BoneOffset = boneOffset;
            inst.BoneCount = boneCount;
            inst.HasAlbedo = sec.AlbedoTexture ? 1u : 0u;
            inst.HasNormal = sec.NormalTexture ? 1u : 0u;
            inst.HasMetallic = sec.MetallicTexture ? 1u : 0u;
            inst.HasRoughness = sec.RoughnessTexture ? 1u : 0u;
            inst.HasAO = sec.AOTexture ? 1u : 0u;
            inst.HasEmissive = sec.EmissiveTexture ? 1u : 0u;
            inst.CustomColor = customColor;

            const uint32_t instanceIndex = static_cast<uint32_t>(mInstanceData.size());
            mInstanceData.push_back(inst);
            mDrawCalls.push_back({ &resource, si, instanceIndex });
        }
    }

    void FbxRenderer::End(ID3D12GraphicsCommandList* cmdList)
    {
        if (mDrawCalls.empty()) return;

        mInstanceBuffer->Update(
            mInstanceData.data(),
            sizeof(FbxInstanceData) * mInstanceData.size());

        if (!mBoneData.empty())
        {
            mBoneBuffer->Update(
                mBoneData.data(),
                sizeof(XMFLOAT4X4) * mBoneData.size());
        }

        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());
        cmdList->SetPipelineState(mPipeline->GetPipelineState());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ── フレーム共通 (1回だけセット) ──────────────────────────
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_INSTANCE_BUFFER, mInstanceBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_BONE_BUFFER, mBoneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_SCENE_BUFFER, mSceneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_LIGHT_BUFFER, mLightBuffer->GetGpuHandle());

        // ── DrawCall ループ ───────────────────────────────────────
        Texture* prevTex[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        const FbxResource* prevResource = nullptr;

        auto BindTex = [&](UINT slot, int cacheIdx, Texture* tex, Texture* fallback)
            {
                Texture* t = tex ? tex : fallback;
                if (t == prevTex[cacheIdx]) return;
                prevTex[cacheIdx] = t;
                if (t) cmdList->SetGraphicsRootDescriptorTable(slot, t->GetGpuHandle());
            };

        for (const DrawCall& dc : mDrawCalls)
        {
            const FbxSection& sec = dc.Resource->GetSections()[dc.SectionIndex];

            // インスタンスインデックスを Root32BitConstant で直接渡す
            // SV_InstanceID + StartInstanceLocation の挙動依存を完全に排除
            cmdList->SetGraphicsRoot32BitConstant(
                FbxPipeline::SLOT_INSTANCE_INDEX, dc.InstanceIndex, 0);

            if (dc.Resource != prevResource)
            {
                dc.Resource->SetBuffers(cmdList);
                prevResource = dc.Resource;
            }

            BindTex(FbxPipeline::SLOT_ALBEDO_TEX, 0, sec.AlbedoTexture, mDefaultWhiteTexture);
            BindTex(FbxPipeline::SLOT_NORMAL_TEX, 1, sec.NormalTexture, mDefaultNormalTexture);
            BindTex(FbxPipeline::SLOT_METALLIC_TEX, 2, sec.MetallicTexture, mDefaultWhiteTexture);
            BindTex(FbxPipeline::SLOT_ROUGHNESS_TEX, 3, sec.RoughnessTexture, mDefaultWhiteTexture);
            BindTex(FbxPipeline::SLOT_AO_TEX, 4, sec.AOTexture, mDefaultWhiteTexture);
            BindTex(FbxPipeline::SLOT_EMISSIVE_TEX, 5, sec.EmissiveTexture, mDefaultBlackTexture);

            // StartInstanceLocation = 0 (SV_InstanceID は使わない)
            cmdList->DrawIndexedInstanced(sec.IndexCount, 1, sec.IndexOffset, 0, 0);
        }
    }

    void FbxRenderer::SetLights(const std::vector<LightData>& lights)
    {
        mLightData = lights;
    }

} // namespace graphics