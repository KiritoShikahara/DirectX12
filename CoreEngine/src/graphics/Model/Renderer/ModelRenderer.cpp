#include "pch.h"
#include "ModelRenderer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<graphics/Shader/ShaderManager.h>
#include<graphics/Texture/Texture.h>

#include<ecs/component/model/ModelComponent.h>
#include<ecs/component/model/ModelAnimComponent.h>
#include<ecs/component/transform/TransformComponent.h>
#include<system/Camera/CameraSystem.h>

namespace graphics
{
    bool ModelRenderer::Initialize(
        DX12Device& device,
        GDescriptorHeapManager& heapManager,
        ShaderManager& shaderManager)
    {
        mPipeline = std::make_unique<ModelPipeline>();
        if (!mPipeline->Create(device, shaderManager))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "ModelRenderer: Failed to create pipeline.");
            return false;
        }

        mInstanceBuffer = std::make_unique<StructuredBuffer>();
        if (!mInstanceBuffer->Create(sizeof(ModelInstanceData), MAX_MODEL_INSTANCES))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "ModelRenderer: Failed to create instance buffer.");
            return false;
        }

        mBoneBuffer = std::make_unique<StructuredBuffer>();
        if (!mBoneBuffer->Create(sizeof(DirectX::XMFLOAT4X4), MAX_TOTAL_BONES))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "ModelRenderer: Failed to create bone buffer.");
            return false;
        }

        mCameraBuffer = std::make_unique<StructuredBuffer>();
        if (!mCameraBuffer->Create(sizeof(ModelCameraData), 1))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "ModelRenderer: Failed to create camera buffer.");
            return false;
        }

        mHeapManager = &heapManager;
        mInstanceData.reserve(MAX_MODEL_INSTANCES);
        mBoneData.reserve(MAX_TOTAL_BONES);
        mDrawCalls.reserve(MAX_MODEL_INSTANCES * 4);

        DEBUG_LOG(sys::eLogLevel::Log, "ModelRenderer: Initialized successfully.");
        return true;
    }

    // Begin
    void ModelRenderer::Begin()
    {
        mInstanceData.clear();
        mBoneData.clear();
        mDrawCalls.clear();
    }

    void ModelRenderer::UpdateAndDraw(entt::registry& registry)
    {
        auto& camSystem = sys::CameraSystem::Get();
        if (!camSystem.HasMainCamera()) return;

        // カメラバッファを今フレームのデータで更新
        // CameraSystem::GetShaderData() は CameraShaderData (FbxData.h) を返すため
        // 同じメモリレイアウトの ModelCameraData にキャストして転送する
        const auto& cam = camSystem.GetShaderData();
        static_assert(sizeof(cam) == sizeof(ModelCameraData),
            "CameraShaderData and ModelCameraData must have identical layout");
        mCameraBuffer->Update(&cam, sizeof(ModelCameraData));

        // Transform + Model コンポーネントを持つエンティティを収集
        auto view = registry.view<ecs::Transform, ecs::Model>();

        struct RenderItem
        {
            const ecs::Transform* transform;
            const ecs::Model* model;
            ecs::ModelAnimComponent* anim;   // nullptr = スキニングなし
        };

        std::vector<RenderItem> items;
        items.reserve(view.size_hint());

        view.each([&](auto entity, ecs::Transform& tr, ecs::Model& mdl)
            {
                if (!mdl.IsVisible || !mdl.Resource || !mdl.Resource->IsLoaded()) return;

                ecs::ModelAnimComponent* anim =
                    registry.try_get<ecs::ModelAnimComponent>(entity);
                items.push_back({ &tr, &mdl, anim });
            });

        // Layer 昇順ソート
        std::sort(items.begin(), items.end(),
            [](const RenderItem& a, const RenderItem& b)
            { return a.model->Layer < b.model->Layer; });

        for (auto& item : items)
        {
            // アニメーションがあればボーン行列を計算
            if (item.anim && item.model->Resource->HasSkinning())
                item.anim->CalcBoneMatrices(*item.model->Resource);

            using namespace DirectX;
            const XMMATRIX world = item.transform->GetWorldMatrix();
            XMFLOAT4X4 worldF;
            XMStoreFloat4x4(&worldF, XMMatrixTranspose(world));

            const std::vector<XMFLOAT4X4>* bonePtr =
                (item.anim && !item.anim->BoneMatrices.empty())
                ? &item.anim->BoneMatrices : nullptr;

            Submit(*item.model->Resource, worldF, item.model->Color, item.model->Intensity, bonePtr);
        }
    }

    void ModelRenderer::Submit(
        const ModelResource& resource,
        const DirectX::XMFLOAT4X4& world,
        const graphics::Color& tint,
        float                                   intensity,
        const std::vector<DirectX::XMFLOAT4X4>* boneMatrices)
    {
        if (mInstanceData.size() >= MAX_MODEL_INSTANCES) return;

        const auto& sections = resource.GetSections();
        const uint32_t instIdx = static_cast<uint32_t>(mInstanceData.size());

        // ボーン行列をプールに追加
        const uint32_t boneOffset = static_cast<uint32_t>(mBoneData.size());
        uint32_t       boneCount = 0;

        if (boneMatrices && !boneMatrices->empty())
        {
            boneCount = static_cast<uint32_t>(boneMatrices->size());
            if (mBoneData.size() + boneCount <= MAX_TOTAL_BONES)
            {
                for (const auto& m : *boneMatrices)
                    mBoneData.push_back(m);
            }
            else
            {
                boneCount = 0;
                DEBUG_LOG(sys::eLogLevel::Warning, "ModelRenderer: Bone buffer overflow.");
            }
        }

        // インスタンスデータ
        ModelInstanceData inst = {};
        inst.World = world;
        inst.BaseColor = { tint.r, tint.g, tint.b, tint.a };
        inst.Metallic = (!sections.empty()) ? sections[0].Metallic : 0.f;
        inst.Roughness = (!sections.empty()) ? sections[0].Roughness : 0.5f;
        inst.Intensity = intensity;
        inst.BoneOffset = boneOffset;
        inst.BoneCount = boneCount;
        mInstanceData.push_back(inst);

        // セクション (マテリアル) ごとに DrawCall 登録
        for (uint32_t s = 0; s < static_cast<uint32_t>(sections.size()); ++s)
            mDrawCalls.push_back({ &resource, s, instIdx });
    }

    void ModelRenderer::End(ID3D12GraphicsCommandList* cmdList)
    {
        if (mDrawCalls.empty()) return;

        // GPU バッファへ一括転送
        mInstanceBuffer->Update(
            mInstanceData.data(),
            sizeof(ModelInstanceData) * mInstanceData.size());

        if (!mBoneData.empty())
            mBoneBuffer->Update(
                mBoneData.data(),
                sizeof(DirectX::XMFLOAT4X4) * mBoneData.size());

        // PSO / RootSignature
        cmdList->SetPipelineState(mPipeline->GetPipelineState());
        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());

        // SRV ヒープをセット
        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        // 全インスタンス共通のバッファをバインド
        cmdList->SetGraphicsRootDescriptorTable(
            ModelPipeline::SLOT_INSTANCE_BUFFER, mInstanceBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            ModelPipeline::SLOT_BONE_BUFFER, mBoneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            ModelPipeline::SLOT_CAMERA_BUFFER, mCameraBuffer->GetGpuHandle());

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const ModelResource* currentResource = nullptr;

        for (const auto& call : mDrawCalls)
        {
            // リソースが変わったら VB/IB を再バインド
            if (call.Resource != currentResource)
            {
                call.Resource->GetVertexBuffer()->Set(cmdList, 0);
                call.Resource->GetIndexBuffer()->Set(cmdList);
                currentResource = call.Resource;
            }

            const ModelSection& sec = call.Resource->GetSections()[call.SectionIndex];

            // ディフューズテクスチャが未解決ならスキップ
            if (!sec.DiffuseTexture) continue;

            cmdList->SetGraphicsRootDescriptorTable(
                ModelPipeline::SLOT_DIFFUSE_TEX, sec.DiffuseTexture->GetGpuHandle());

            // 法線マップが存在すればバインド (なければそのまま)
            if (sec.NormalTexture)
            {
                cmdList->SetGraphicsRootDescriptorTable(
                    ModelPipeline::SLOT_NORMAL_TEX, sec.NormalTexture->GetGpuHandle());
            }

            // DrawIndexedInstanced:
            //   instanceCount = 1, startInstance = instanceIndex
            //   HLSL 側 SV_InstanceID が mInstanceData[instanceIndex] を参照する
            cmdList->DrawIndexedInstanced(
                sec.IndexCount,
                1,
                sec.IndexOffset,
                0,
                call.InstanceIndex);
        }
    }

}