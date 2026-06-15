#include "pch.h"
#include "SkyboxRenderer.h"

#include<ecs/component/skybox/SkyboxComponent.h>

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<graphics/Texture/TextureManager.h>
#include<graphics/Texture/Texture.h>

namespace graphics
{
	bool SkyboxRenderer::Initialize()
	{
		mPipeline = std::make_unique<SkyboxPipeline>();
		if (mPipeline->Create() == false)
		{
			return false;
		}


		mHeapManager = &graphics::GDescriptorHeapManager::Get();
		DEBUG_LOG(sys::eLogLevel::Log, "SkyboxRenderer: Initialized successfully.");
		return true;
	}

	void SkyboxRenderer::Begin()
	{
		mTexA = nullptr;
		mTexB = nullptr;
		mBlendWeight = 0.0f;
		mHasDraw = false;
	}

	void SkyboxRenderer::UpdateAndDraw(entt::registry& registry)
	{
        struct Entry
        {
            int                          Priority;
            float                        Weight;
            const std::filesystem::path* TexturePath;
        };

        std::vector<Entry> entries;
        entries.reserve(8);

        registry.view<ecs::SkyboxComponent>().each(
            [&](const ecs::SkyboxComponent& comp)
            {
                entries.push_back({ comp.Priority, comp.Weight, &comp.TexturePath });
            });

        if (entries.empty()) return;

        std::sort(entries.begin(), entries.end(),
            [](const Entry& a, const Entry& b) { return a.Priority > b.Priority; });

        auto& texMgr = TextureManager::Get();

        // A 側 (最高優先度)
        const Texture* texA = texMgr.GetOrLoad(*entries[0].TexturePath);
        if (!texA || !texA->IsValid())
        {
            DEBUG_LOG(sys::eLogLevel::Warning, "SkyboxRenderer: Texture A is invalid, skip draw.");
            return;
        }

        // B 側 (2番目。存在しない場合は A と同じテクスチャで Weight=0)
        const Texture* texB = texA;
        float          weight = 0.0f;

        if (entries.size() >= 2)
        {
            const Texture* candidate = texMgr.GetOrLoad(*entries[1].TexturePath);
            if (candidate && candidate->IsValid())
            {
                texB = candidate;
                weight = std::clamp(entries[1].Weight, 0.0f, 1.0f);
            }
        }

        mTexA = texA;
        mTexB = texB;
        mBlendWeight = weight;
        mHasDraw = true;
	}

    void SkyboxRenderer::End(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE sceneBufferGpuHandle)
    {
        if (!mHasDraw) return;

        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());
        cmdList->SetPipelineState(mPipeline->GetPipelineState());

        // ヒープ (FbxRenderer::End() と同じヒープを使い回す)
        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        // SLOT_BLEND_WEIGHT (b0): BlendWeight を float 1個として渡す
        cmdList->SetGraphicsRoot32BitConstant(
            SkyboxPipeline::SLOT_BLEND_WEIGHT,
            *reinterpret_cast<const UINT*>(&mBlendWeight),
            0);

        // SLOT_SCENE_BUFFER (t8, space0): FbxRenderer::mSceneBuffer の GPU ハンドルを共有
        cmdList->SetGraphicsRootDescriptorTable(
            SkyboxPipeline::SLOT_SCENE_BUFFER,
            sceneBufferGpuHandle);

        // SLOT_SKYBOX_TEX_A (t0, space1): キューブマップ A
        cmdList->SetGraphicsRootDescriptorTable(
            SkyboxPipeline::SLOT_SKYBOX_TEX_A,
            mTexA->GetGpuHandle());

        // SLOT_SKYBOX_TEX_B (t1, space1): キューブマップ B
        cmdList->SetGraphicsRootDescriptorTable(
            SkyboxPipeline::SLOT_SKYBOX_TEX_B,
            mTexB->GetGpuHandle());

        // VB / IB なし
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 0, nullptr);
        cmdList->IASetIndexBuffer(nullptr);

        // フルスクリーントライアングル (3頂点, 1インスタンス)
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
}