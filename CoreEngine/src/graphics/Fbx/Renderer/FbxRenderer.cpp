#include"pch.h"
#include "FbxRenderer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<graphics/Shader/ShaderManager.h>
#include<graphics/Texture/Texture.h>

#include<ecs/component/fbx/FbxComponent.h>
#include<ecs/component/fbx/AnimationComponent.h>
#include<ecs/component/transform/TransformComponent.h>
#include<system/Camera/CameraSystem.h>

namespace graphics
{
    bool FbxRenderer::Initialize(
        DX12Device& device,
        GDescriptorHeapManager& heapManager,
        ShaderManager& shaderManager)
    {
        // パイプライン
        mPipeline = std::make_unique<FbxPipeline>();
        if (!mPipeline->Create(device, shaderManager))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create pipeline.");
            return false;
        }

        // インスタンスデータバッファ（FbxInstanceData × MAX_FBX_INSTANCES）
        mInstanceBuffer = std::make_unique<StructuredBuffer>();
        if (!mInstanceBuffer->Create(sizeof(FbxInstanceData), MAX_FBX_INSTANCES))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create instance buffer.");
            return false;
        }

        // ボーン行列プールバッファ（XMFLOAT4X4 × MAX_TOTAL_BONES）
        mBoneBuffer = std::make_unique<StructuredBuffer>();
        if (!mBoneBuffer->Create(sizeof(DirectX::XMFLOAT4X4), MAX_TOTAL_BONES))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create bone buffer.");
            return false;
        }

        mCameraBuffer = std::make_unique<StructuredBuffer>();
        if (!mCameraBuffer->Create(sizeof(CameraShaderData), 1))
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "FbxRenderer: Failed to create camera buffer.");
            return false;
        }

        mHeapManager = &heapManager;

        mInstanceData.reserve(MAX_FBX_INSTANCES);
        mBoneData.reserve(MAX_TOTAL_BONES);
        mDrawCalls.reserve(MAX_FBX_INSTANCES * 4);  // 1 モデルあたり平均 4 マテリアルを想定

        DEBUG_LOG(sys::eLogLevel::Log, "FbxRenderer: Initialized successfully.");
        return true;
    }

    /// <summary>
    /// データのクリア
    /// </summary>
    void FbxRenderer::Begin()
    {
        mInstanceData.clear();
        mBoneData.clear();
        mDrawCalls.clear();
    }

    void FbxRenderer::UpdateAndDraw(entt::registry& registry)
    {
        // カメラ確認・行列更新は CameraSystem に委譲
        auto& camSystem = sys::CameraSystem::Get();
        if (!camSystem.HasMainCamera()) return;

        // カメラバッファを今フレームのデータで更新
        const auto& camData = camSystem.GetShaderData();
        mCameraBuffer->Update(&camData, sizeof(CameraShaderData));

        // Transform + FbxModel を持つエンティティを収集
        auto view = registry.view<ecs::Transform, ecs::FbxModel>();

        struct RenderItem
        {
            const ecs::Transform* transform;
            const ecs::FbxModel* model;
            ecs::AnimationComponent* anim;   // nullptr = スキニングなし
        };

        std::vector<RenderItem> items;
        items.reserve(view.size_hint());

        view.each([&](auto entity, ecs::Transform& tr, ecs::FbxModel& mdl)
            {
                if (!mdl.IsVisible || !mdl.Resource || !mdl.Resource->IsLoaded()) return;

                // AnimationComponent があれば取得（オプション）
                ecs::AnimationComponent* anim =
                    registry.try_get<ecs::AnimationComponent>(entity);

                items.push_back({ &tr, &mdl, anim });
            });

        // Layer 昇順ソート（スプライトと共通の描画順ルール）
        std::sort(items.begin(), items.end(),
            [](const RenderItem& a, const RenderItem& b)
            {
                return a.model->Layer < b.model->Layer;
            });

        // アニメーション更新 + サブミット
        // ※ deltaTime は外部から渡すことが理想だが、
        //   ECS システムとして分離する場合は Update を別システムで行い
        //   ここでは CalcBoneMatrices のみ呼ぶ設計も可能
        for (auto& item : items)
        {
            if (item.anim && item.model->Resource->HasAnimation())
            {
                item.anim->CalcBoneMatrices(*item.model->Resource);
            }

            using namespace DirectX;

            // ワールド行列（転置はシェーダデータ格納時に行う）
            const XMMATRIX world = item.transform->GetWorldMatrix();
            XMFLOAT4X4 worldF;
            XMStoreFloat4x4(&worldF, XMMatrixTranspose(world));

            const std::vector<XMFLOAT4X4>* bonePtr =
                (item.anim && !item.anim->BoneMatrices.empty())
                ? &item.anim->BoneMatrices
                : nullptr;

            Submit(*item.model->Resource, worldF, item.model->Color, item.model->Intensity, bonePtr);
        }
    }

    void FbxRenderer::Submit(
        const FbxResource& resource,
        const DirectX::XMFLOAT4X4& world,
        const graphics::Color& tint,
        float                                       intensity,
        const std::vector<DirectX::XMFLOAT4X4>* boneMatrices)
    {
        if (mInstanceData.size() >= MAX_FBX_INSTANCES) return;

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
                {
                    mBoneData.push_back(m);
                }
            }
            else
            {
                boneCount = 0;  // バッファ溢れの場合はスキニングなしにフォールバック
                DEBUG_LOG(sys::eLogLevel::Warning, "FbxRenderer: Bone buffer overflow. Skinning skipped.");
            }
        }

        // インスタンスデータを生成（マテリアルパラメータは最初のセクションから取る）
        // セクション毎にテクスチャは異なるが WVP / ボーンは共通のためインスタンスは 1 つ
        FbxInstanceData inst = {};
        inst.World = world;
        inst.BaseColor = { tint.r, tint.g, tint.b, tint.a };
        inst.Metallic = (!sections.empty()) ? sections[0].Metallic : 0.f;
        inst.Roughness = (!sections.empty()) ? sections[0].Roughness : 0.5f;
        inst.Intensity = intensity;
        inst.BoneOffset = boneOffset;
        inst.BoneCount = boneCount;
        mInstanceData.push_back(inst);

        // セクション毎にドローコールを登録
        for (uint32_t s = 0; s < static_cast<uint32_t>(sections.size()); ++s)
        {
            mDrawCalls.push_back({ &resource, s, instIdx });
        }
    }

    void FbxRenderer::End(ID3D12GraphicsCommandList* cmdList)
    {
        if (mDrawCalls.empty() || !sys::CameraSystem::Get().HasMainCamera()) return;

        // --- GPU バッファへ一括転送 ---
        mInstanceBuffer->Update(
            mInstanceData.data(),
            sizeof(FbxInstanceData) * mInstanceData.size());

        if (!mBoneData.empty())
        {
            mBoneBuffer->Update(
                mBoneData.data(),
                sizeof(DirectX::XMFLOAT4X4) * mBoneData.size());
        }

        // --- PSO / RootSignature ---
        cmdList->SetPipelineState(mPipeline->GetPipelineState());
        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());

        // --- SRV ヒープのセット ---
        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_INSTANCE_BUFFER, mInstanceBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_BONE_BUFFER, mBoneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_CAMERA_BUFFER, mCameraBuffer->GetGpuHandle());

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const FbxResource* currentResource = nullptr;

        for (const auto& call : mDrawCalls)
        {
            // リソースが変わったらバッファを再バインド
            if (call.Resource != currentResource)
            {
                call.Resource->GetVertexBuffer()->Set(cmdList, 0);
                call.Resource->GetIndexBuffer()->Set(cmdList);
                currentResource = call.Resource;
            }

            const FbxSection& sec = call.Resource->GetSections()[call.SectionIndex];

            // テクスチャが未解決の場合はスキップ
            if (!sec.DiffuseTexture) continue;

            // --- [t2] セクション毎のテクスチャ差し替え ---
            cmdList->SetGraphicsRootDescriptorTable(
                FbxPipeline::SLOT_DIFFUSE_TEX,
                sec.DiffuseTexture->GetGpuHandle());

            // DrawIndexedInstanced: instanceCount=1, startInstance=instanceIndex
            // シェーダ側は SV_InstanceID で mInstanceData[instanceIndex] を参照する
            cmdList->DrawIndexedInstanced(
                sec.IndexCount,      // 描画インデックス数
                1,                   // インスタンス数
                sec.IndexOffset,     // インデックスバッファ先頭からのオフセット
                0,                   // 頂点バッファオフセット
                call.InstanceIndex); // SV_InstanceID にマッピングされる値
        }
    }

}