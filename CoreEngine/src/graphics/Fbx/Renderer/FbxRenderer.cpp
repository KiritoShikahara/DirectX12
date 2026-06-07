#include "pch.h"
#include "FbxRenderer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Texture/Texture.h>
#include<graphics/Shader/ShaderManager.h>
#include<graphics/Fbx/Resource/FbxResource.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<graphics/Texture/TextureManager.h>

#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/Fbx/FbxComponent.h>
#include<ecs/component/Fbx/FbxAnimComponent.h>
#include<system/Camera/CameraSystem.h>

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

        // BoneBuffer: float4x4 × MAX_TOTAL_BONES  (全エンティティ分を連結)
        mBoneBuffer = std::make_unique<StructuredBuffer>();
        if (!mBoneBuffer->Create(sizeof(XMFLOAT4X4), MAX_TOTAL_BONES))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create bone buffer.");
            return false;
        }

        // SceneBuffer: FbxSceneData × 1
        mSceneBuffer = std::make_unique<StructuredBuffer>();
        if (!mSceneBuffer->Create(sizeof(FbxSceneData), 1))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create scene buffer.");
            return false;
        }

        auto& textureManager = TextureManager::Get();

        mDefaultWhiteTexture = textureManager.GetOrLoad(
            ASSET_PATH("/Engine/Assets/Texture/White.dds").string());
        mDefaultNormalTexture = textureManager.GetOrLoad(
            ASSET_PATH("/Engine/Assets/Texture/Normal.dds").string());
        mDefaultBlackTexture = textureManager.GetOrLoad(
            ASSET_PATH("/Engine/Assets/Texture/Black.dds").string());

        mLightBuffer = std::make_unique<StructuredBuffer>();
        mLightBuffer->Create(sizeof(LightData), MAX_LIGHTS);

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

        // カメラ + ライトデータをシーンバッファへ書き込む
        // CameraSystem::GetShaderData() は ViewProjection(転置済み) + Position を返す
        const auto& cam = camSystem.GetShaderData();
        FbxSceneData scene = {};
        std::memcpy(&scene.ViewProjection, &cam.ViewProjection, sizeof(XMFLOAT4X4));
        scene.CameraPosition = cam.Position;
        scene.LightCount = static_cast<uint32_t>(mLightData.size());
        mSceneBuffer->Update(&scene, sizeof(FbxSceneData));

        // LightBuffer 書き込み
        if (!mLightData.empty())
        {
            mLightBuffer->Update(
                mLightData.data(),
                sizeof(LightData) * mLightData.size());
        }

        // Transform + FbxComponent を持つエンティティを収集
        struct RenderItem
        {
            const ecs::Transform* transform;
            const ecs::FbxComponent* fbx;
            ecs::FbxAnimComponent* anim;   // nullptr = スキニングなし
        };

        std::vector<RenderItem> items;
        auto view = registry.view<ecs::Transform, ecs::FbxComponent>();
        items.reserve(view.size_hint());

        view.each([&](auto entity, ecs::Transform& tr, ecs::FbxComponent& fbxComp)
            {
                if (!fbxComp.IsVisible || !fbxComp.Resource || !fbxComp.Resource->IsLoaded()) return;

                ecs::FbxAnimComponent* anim =
                    registry.try_get<ecs::FbxAnimComponent>(entity);
                items.push_back({ &tr, &fbxComp, anim });
            });

        for (auto& item : items)
        {
            // アニメーションがあればボーン行列を更新
            if (item.anim && item.fbx->Resource->HasSkinning())
                item.anim->CalcBoneMatrices(*item.fbx->Resource);

            XMFLOAT4X4 worldF;
            XMStoreFloat4x4(&worldF, XMMatrixTranspose(item.transform->GetWorldMatrix()));

            const std::vector<XMFLOAT4X4>* bonePtr =
                (item.anim && !item.anim->BoneMatrices.empty())
                ? &item.anim->BoneMatrices
                : nullptr;

            Submit(*item.fbx->Resource, worldF, bonePtr);
        }
    }

    void FbxRenderer::Submit(
        const FbxResource& resource,
        const XMFLOAT4X4& world,
        const std::vector<XMFLOAT4X4>* boneMatrices)
    {
        // ボーンデータをグローバルBoneBufferに連結して追記
        const uint32_t boneOffset = static_cast<uint32_t>(mBoneData.size());
        uint32_t       boneCount = 0u;

        if (boneMatrices && !boneMatrices->empty() && resource.HasSkinning())
        {
            boneCount = static_cast<uint32_t>(boneMatrices->size());
            if (boneOffset + boneCount <= MAX_TOTAL_BONES)
            {
                // FbxAnimComponent::CalcBoneMatrices() が転置済みで格納するため
                // ここでは memcpy 相当の挿入のみ行う (ModelRenderer と同方針)
                mBoneData.insert(mBoneData.end(), boneMatrices->begin(), boneMatrices->end());
            }
            else
            {
                DEBUG_LOG(sys::eLogLevel::Warning, "FbxRenderer: BoneBuffer overflow. Skinning skipped.");
                boneCount = 0u;
            }
        }

        // セクション毎にインスタンスデータ + DrawCall を生成
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

            const uint32_t instanceIndex = static_cast<uint32_t>(mInstanceData.size());
            mInstanceData.push_back(inst);

            mDrawCalls.push_back({ &resource, si, instanceIndex });
        }
    }

    void FbxRenderer::End(ID3D12GraphicsCommandList* cmdList)
    {
        if (mDrawCalls.empty()) return;

        // InstanceBuffer と BoneBuffer を今フレームのデータで更新
        mInstanceBuffer->Update(
            mInstanceData.data(),
            sizeof(FbxInstanceData) * mInstanceData.size());

        if (!mBoneData.empty())
        {
            mBoneBuffer->Update(
                mBoneData.data(),
                sizeof(XMFLOAT4X4) * mBoneData.size());
        }

        // デスクリプタヒープのセット
        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());
        cmdList->SetPipelineState(mPipeline->GetPipelineState());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ── フレーム共通スロット (1回だけセット) ──────────────────
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_INSTANCE_BUFFER, mInstanceBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_BONE_BUFFER, mBoneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_SCENE_BUFFER, mSceneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_LIGHT_BUFFER, mLightBuffer->GetGpuHandle());

        // ── DrawCall ループ ───────────────────────────────────────
        // テクスチャ変更時のみ SetGraphicsRootDescriptorTable を呼ぶよう
        // 前回のバインドをキャッシュしてバインド回数を削減する
        Texture* prevTex[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        const FbxResource* prevResource = nullptr;

        auto BindTex = [&](UINT slot, int cacheIdx, Texture* tex, Texture* fallback)
            {
                Texture* t = tex ? tex : fallback;
                if (t == prevTex[cacheIdx]) return;   // 変化なければスキップ
                prevTex[cacheIdx] = t;
                if (t) cmdList->SetGraphicsRootDescriptorTable(slot, t->GetGpuHandle());
            };

        for (const DrawCall& dc : mDrawCalls)
        {
            const FbxSection& sec = dc.Resource->GetSections()[dc.SectionIndex];

            // VB/IB はリソースが変わった時だけ再セット
            if (dc.Resource != prevResource)
            {
                dc.Resource->SetBuffers(cmdList);
                prevResource = dc.Resource;
            }

            // PBR テクスチャをセット (変化があるスロットのみ)
            BindTex(FbxPipeline::SLOT_ALBEDO_TEX, 0, sec.AlbedoTexture, mDefaultWhiteTexture);
            BindTex(FbxPipeline::SLOT_NORMAL_TEX, 1, sec.NormalTexture, mDefaultNormalTexture);
            BindTex(FbxPipeline::SLOT_METALLIC_TEX, 2, sec.MetallicTexture, mDefaultWhiteTexture);
            BindTex(FbxPipeline::SLOT_ROUGHNESS_TEX, 3, sec.RoughnessTexture, mDefaultWhiteTexture);
            BindTex(FbxPipeline::SLOT_AO_TEX, 4, sec.AOTexture, mDefaultWhiteTexture);
            BindTex(FbxPipeline::SLOT_EMISSIVE_TEX, 5, sec.EmissiveTexture, mDefaultBlackTexture);

            // StartInstanceLocation = dc.InstanceIndex
            // → シェーダーで SV_InstanceID = dc.InstanceIndex となり
            //   InstanceBuffer[SV_InstanceID] で正しいデータを参照できる
            cmdList->DrawIndexedInstanced(
                sec.IndexCount, 1, sec.IndexOffset, 0, dc.InstanceIndex);
        }
    }

    void FbxRenderer::SetLights(const std::vector<LightData>& lights)
    {
        mLightData = lights;
    }
}