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

#include <d3dx12.h>

using namespace DirectX;

namespace graphics
{
    // ============================================================
    //  Initialize
    // ============================================================
    bool FbxRenderer::Initialize(
        DX12Device& device,
        GDescriptorHeapManager& heapManager,
        ShaderManager& shaderManager)
    {
        mHeapManager = &heapManager;

        // ── FBX 通常パスパイプライン ──────────────────────────────
        mPipeline = std::make_unique<FbxPipeline>();
        if (!mPipeline->Create(device, shaderManager))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create FbxPipeline.");
            return false;
        }

        // ── Shadow パスパイプライン (Root Signature は FbxPipeline と共有) ──
        mShadowPipeline = std::make_unique<ShadowPipeline>();
        if (!mShadowPipeline->Create(device, shaderManager, mPipeline->GetRootSignature()))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create ShadowPipeline.");
            return false;
        }

        // ── GPU バッファ ──────────────────────────────────────────
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

        // ── Shadow Map リソース ───────────────────────────────────
        if (!CreateShadowMapResources(device, heapManager))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create shadow map resources.");
            return false;
        }

        // ── デフォルトテクスチャ ──────────────────────────────────
        auto& texMgr = TextureManager::Get();
        mDefaultWhiteTexture = texMgr.GetOrLoad(ASSET_PATH("/Engine/Assets/Texture/White.dds").string());
        mDefaultNormalTexture = texMgr.GetOrLoad(ASSET_PATH("/Engine/Assets/Texture/Normal.dds").string());
        mDefaultBlackTexture = texMgr.GetOrLoad(ASSET_PATH("/Engine/Assets/Texture/Black.dds").string());

        mInstanceData.reserve(MAX_FBX_INSTANCES);
        mBoneData.reserve(MAX_TOTAL_BONES);
        mDrawCalls.reserve(MAX_FBX_INSTANCES * 4);

        // 距離LODの切り替えUI(開発ツール有効時のみ。PhysicsDebugRendererと同じ流儀)
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().AddDebugUI([this]() { ImGuiWindow(); }, "FbxRenderer");
#endif

        DEBUG_LOG(sys::eLogLevel::Log, "FbxRenderer: Initialized successfully.");
        return true;
    }

    void FbxRenderer::ImGuiWindow()
    {
#if DEV_TOOL_ENABLED
        if (ImGui::Begin("Rendering"))
        {
            ImGui::TextUnformatted("Skinned Animation LOD");
            ImGui::Checkbox("Distance LOD", &mAnimationDistanceLodEnabled);

            // このファイルはBOM無しのため、文字列リテラルに日本語を使うと
            // コードページ932として誤解釈されコンパイルが壊れる。
            // 既存のデバッグUI(PhysicsDebugRenderer等)と同じく表示は英語で統一する。
            if (mAnimationDistanceLodEnabled)
            {
                ImGui::DragFloat("Update Distance (m)", &mAnimationUpdateDistance, 1.0f, 1.0f, 1000.0f);
                ImGui::TextDisabled("Characters beyond this distance");
                ImGui::TextDisabled("appear frozen (animation skipped).");
            }
            else
            {
                ImGui::TextDisabled("Disabled (default): all characters update.");
            }
        }
        ImGui::End();
#endif
    }

    void FbxRenderer::Finalize()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI("FbxRenderer");
#endif

        mShadowMapSRV.Release();
        mShadowMapNullSRV.Release();
        mInstanceBuffer.reset();
        mBoneBuffer.reset();
        mSceneBuffer.reset();
        mLightBuffer.reset();

        mShadowMapResource.Reset();
        mShadowMapDSVHeap.Reset();
        mShadowMapSRV.Release();
        mShadowMapNullSRV.Release();

        mPipeline.reset();
        mShadowPipeline.reset();
    }

    // ============================================================
    //  Shadow Map リソース生成
    // ============================================================
    bool FbxRenderer::CreateShadowMapResources(
        DX12Device& device,
        GDescriptorHeapManager& heapManager)
    {
        ID3D12Device* d3d = device.GetDevice();

        // ── テクスチャリソース ────────────────────────────────────
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = SHADOW_MAP_SIZE;
        texDesc.Height = SHADOW_MAP_SIZE;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R32_TYPELESS;     // DSV=D32_FLOAT / SRV=R32_FLOAT
        texDesc.SampleDesc.Count = 1;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_CLEAR_VALUE clearVal = {};
        clearVal.Format = DXGI_FORMAT_D32_FLOAT;
        clearVal.DepthStencil.Depth = 1.0f;
        clearVal.DepthStencil.Stencil = 0;

        HRESULT hr = d3d->CreateCommittedResource(
            &heapProp,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,  // 初期状態: PS SRV
            &clearVal,
            IID_PPV_ARGS(&mShadowMapResource));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create shadow map texture.");
            return false;
        }
        mShadowMapResource->SetName(L"ShadowMapTexture");

        // ── DSV ヒープ (非シェーダービジブル) ─────────────────────
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        hr = d3d->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mShadowMapDSVHeap));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to create DSV heap.");
            return false;
        }

        // DSV ビュー
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        d3d->CreateDepthStencilView(
            mShadowMapResource.Get(),
            &dsvDesc,
            mShadowMapDSVHeap->GetCPUDescriptorHandleForHeapStart());

        // ── SRV (GDescriptorHeapManager 管理) ─────────────────────
        if (!mShadowMapSRV.Create(heapManager, 1))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to allocate shadow map SRV slot.");
            return false;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        d3d->CreateShaderResourceView(
            mShadowMapResource.Get(),
            &srvDesc,
            mShadowMapSRV.GetCpuHandle());

        // ── Null SRV (Shadow Map なし時のフォールバック) ───────────
        // Shadow Map が未使用フレームでも t10 スロットに有効なディスクリプタが必要
        if (!mShadowMapNullSRV.Create(heapManager, 1))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxRenderer: Failed to allocate null SRV slot.");
            return false;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
        nullSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullSrvDesc.Texture2D.MipLevels = 1;
        // リソースを nullptr にすることで null descriptor を作成
        d3d->CreateShaderResourceView(nullptr, &nullSrvDesc, mShadowMapNullSRV.GetCpuHandle());

        DEBUG_LOG(sys::eLogLevel::Log, "FbxRenderer: Shadow map resources created ({}x{}).",
            SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        return true;
    }

    // ============================================================
    //  Begin
    // ============================================================
    void FbxRenderer::Begin()
    {
        mInstanceData.clear();
        mBoneData.clear();
        mDrawCalls.clear();
    }

    // ============================================================
    //  UpdateAndDraw  (収集フェーズ)
    //
    //  registry から描画対象を収集し、GPU バッファへの転送まで完結させる。
    //  DrawShadowPass() / End() は記録のみを行うため、
    //  Update() を呼ぶのはこの関数だけ。
    // ============================================================
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

        // 毎フレームのvector生成を避けるため、メンバ変数(mRenderItems)を使い回す
        mRenderItems.clear();
        auto view = registry.view<ecs::Transform, ecs::FbxComponent>();
        mRenderItems.reserve(view.size_hint());

        view.each([&](auto entity, ecs::Transform& tr, ecs::FbxComponent& fbxComp)
            {
                if (!fbxComp.IsVisible || !fbxComp.Resource || !fbxComp.Resource->IsLoaded()) return;
                ecs::FbxAnimComponent* anim = registry.try_get<ecs::FbxAnimComponent>(entity);
                mRenderItems.push_back({ &tr, &fbxComp, anim });
            });

        for (auto& item : mRenderItems)
        {
            const bool hasAnimation = (item.Anim && item.Fbx->Resource->HasSkinning());
            if (hasAnimation)
            {
                // 距離LODが無効(既定)なら常に計算する。
                // 有効時はカメラから遠いエンティティ(画面外含む)のボーン行列再計算を間引き、
                // 最後に計算済みの姿勢のまま描画する。BoneMatricesが一度も計算されていない
                // 場合(生成直後)は、Tポーズのまま描画されるのを避けるため必ず計算する。
                bool needsCalc = true;

                if (mAnimationDistanceLodEnabled && !item.Anim->BoneMatrices.empty())
                {
                    const XMFLOAT3& pos = item.Transform->GetPosition();
                    const float dx = pos.x - scene.CameraPosition.x;
                    const float dy = pos.y - scene.CameraPosition.y;
                    const float dz = pos.z - scene.CameraPosition.z;
                    const float distSq = dx * dx + dy * dy + dz * dz;

                    needsCalc = (distSq <= mAnimationUpdateDistance * mAnimationUpdateDistance);
                }

                if (needsCalc)
                    item.Anim->CalcBoneMatrices(*item.Fbx->Resource);
            }

            XMFLOAT3 pivot = { 0.f, 0.f, 0.f };
            if (!hasAnimation)
            {
                pivot = item.Fbx->AutoPivot
                    ? item.Fbx->Resource->GetBottomCenterPivot()
                    : item.Fbx->PivotOffset;
            }

            XMMATRIX world = item.Transform->GetWorldMatrix();
            if (pivot.x != 0.f || pivot.y != 0.f || pivot.z != 0.f)
                world = XMMatrixTranslation(pivot.x, pivot.y, pivot.z) * world;

            XMFLOAT4X4 worldF;
            XMStoreFloat4x4(&worldF, XMMatrixTranspose(world));

            const std::vector<XMFLOAT4X4>* bonePtr =
                (hasAnimation && !item.Anim->BoneMatrices.empty())
                ? &item.Anim->BoneMatrices
                : nullptr;

            Submit(*item.Fbx->Resource, worldF, bonePtr, item.Fbx->CustomColor);
        }

        // ── GPU バッファへの転送 ──────────────────────────────────
        // Submit() で mInstanceData / mBoneData が確定した後に一度だけ転送する。
        // DrawShadowPass() と End() の両方がこのデータを参照するが、
        // どちらも記録のみを行うため転送はここに集約する。
        if (!mInstanceData.empty())
        {
            mInstanceBuffer->Update(
                mInstanceData.data(),
                sizeof(FbxInstanceData) * mInstanceData.size());
        }

        if (!mBoneData.empty())
        {
            mBoneBuffer->Update(
                mBoneData.data(),
                sizeof(XMFLOAT4X4) * mBoneData.size());
        }
    }

    // ============================================================
    //  Submit  (変更なし)
    // ============================================================
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
                mBoneData.insert(mBoneData.end(), boneMatrices->begin(), boneMatrices->end());
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

    // ============================================================
    //  DrawShadowPass  (記録フェーズ)
    //
    //  UpdateAndDraw で蓄積した DrawCall をライト空間で深度描画する。
    //  OMSetRenderTargets(0, nullptr) で RTV を外し、ビューポートも
    //  SHADOW_MAP_SIZE に変更するが、これらはコマンドリスト単位の状態のため
    //  専用チャネル (eRenderChannel::Shadow) に隔離されている限り
    //  他チャネルには影響しない (RestoreMainRenderTarget は不要)。
    // ============================================================
    void FbxRenderer::DrawShadowPass(ID3D12GraphicsCommandList* cmdList)
    {
        // CastShadow == true の Directional Light が index 0 にあるか確認
        if (mLightData.empty()) return;
        const LightData& shadowLight = mLightData[0];
        if (shadowLight.Type != 0 /*DIRECTIONAL*/ || !shadowLight.CastShadow) return;
        if (mDrawCalls.empty()) return;

        // GPU バッファは UpdateAndDraw() で転送済み。
        // この関数はコマンドの記録のみを行い、registry / GPU バッファには触れない。

        // ── Shadow Map を DSV として使えるようにバリア ────────────
        auto barrierToDSV = CD3DX12_RESOURCE_BARRIER::Transition(
            mShadowMapResource.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmdList->ResourceBarrier(1, &barrierToDSV);

        // ── DSV クリア ────────────────────────────────────────────
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
            mShadowMapDSVHeap->GetCPUDescriptorHandleForHeapStart();
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // ── RenderTarget なし / DSV のみをセット ─────────────────
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

        // ── ビューポート / シザー ─────────────────────────────────
        D3D12_VIEWPORT vp = { 0.f, 0.f,
            static_cast<float>(SHADOW_MAP_SIZE),
            static_cast<float>(SHADOW_MAP_SIZE),
            0.f, 1.f };
        D3D12_RECT scissor = { 0, 0,
            static_cast<LONG>(SHADOW_MAP_SIZE),
            static_cast<LONG>(SHADOW_MAP_SIZE) };
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &scissor);

        // ── パイプライン / Root Signature ────────────────────────
        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());
        cmdList->SetPipelineState(mShadowPipeline->GetPipelineState());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ── フレーム共通バッファをセット ──────────────────────────
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_INSTANCE_BUFFER, mInstanceBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_BONE_BUFFER, mBoneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_LIGHT_BUFFER, mLightBuffer->GetGpuHandle());

        // Shadow Pass 用ライトインデックス (index 0 固定)
        cmdList->SetGraphicsRoot32BitConstant(
            FbxPipeline::SLOT_SHADOW_LIGHT_INDEX, 0u, 0);

        // ── DrawCall ループ ───────────────────────────────────────
        const FbxResource* prevResource = nullptr;

        for (const DrawCall& dc : mDrawCalls)
        {
            cmdList->SetGraphicsRoot32BitConstant(
                FbxPipeline::SLOT_INSTANCE_INDEX, dc.InstanceIndex, 0);

            if (dc.Resource != prevResource)
            {
                dc.Resource->SetBuffers(cmdList);
                prevResource = dc.Resource;
            }

            const FbxSection& sec = dc.Resource->GetSections()[dc.SectionIndex];
            cmdList->DrawIndexedInstanced(sec.IndexCount, 1, sec.IndexOffset, 0, 0);
        }

        // ── Shadow Map を SRV (PS 読み取り) に戻す ────────────────
        auto barrierToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
            mShadowMapResource.Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrierToSRV);
    }

    // ============================================================
    //  End  (通常描画パス / 記録フェーズ)
    //
    //  GPU バッファは UpdateAndDraw() で転送済み。
    //  この関数はコマンドの記録のみを行い、registry / GPU バッファには触れない。
    // ============================================================
    void FbxRenderer::End(ID3D12GraphicsCommandList* cmdList)
    {
        if (mDrawCalls.empty()) return;

        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());
        cmdList->SetPipelineState(mPipeline->GetPipelineState());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ── フレーム共通 ───────────────────────────────────────────
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_INSTANCE_BUFFER, mInstanceBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_BONE_BUFFER, mBoneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_SCENE_BUFFER, mSceneBuffer->GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_LIGHT_BUFFER, mLightBuffer->GetGpuHandle());

        // Shadow Map SRV: CastShadow ライトがあれば実体、なければ Null SRV
        const bool hasShadow = !mLightData.empty()
            && mLightData[0].Type == 0
            && mLightData[0].CastShadow;

        cmdList->SetGraphicsRootDescriptorTable(
            FbxPipeline::SLOT_SHADOW_MAP,
            hasShadow ? mShadowMapSRV.GetGpuHandle() : mShadowMapNullSRV.GetGpuHandle());

        // ── DrawCall ループ ───────────────────────────────────────
        Texture* prevTex[6] = {};
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

            cmdList->DrawIndexedInstanced(sec.IndexCount, 1, sec.IndexOffset, 0, 0);
        }
    }

    // ============================================================
    //  SetLights
    // ============================================================
    void FbxRenderer::SetLights(const std::vector<LightData>& lights)
    {
        mLightData = lights;
    }

} // namespace graphics