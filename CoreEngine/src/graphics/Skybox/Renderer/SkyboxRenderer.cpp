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

    void SkyboxRenderer::Finalize()
    {
        if (mPipeline)
        {
            // 必要に応じて SkyboxPipeline 側にもリセット処理を追加
            mPipeline.reset();
        }

        // Texture などの参照もクリア
        mTexA = nullptr;
        mTexB = nullptr;

        DEBUG_LOG(sys::eLogLevel::Log, "SkyboxRenderer: Finalized.");
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
        // 毎フレームのvector生成を避けるため、メンバ変数を使い回す
        mEntries.clear();

        registry.view<ecs::SkyboxComponent>().each(
            [&](const ecs::SkyboxComponent& comp)
            {
                mEntries.push_back({ comp.Priority, comp.Weight, &comp.TexturePath });
            });

        if (mEntries.empty()) return;

        std::sort(mEntries.begin(), mEntries.end(),
            [](const Entry& a, const Entry& b) { return a.Priority > b.Priority; });

        auto& texMgr = TextureManager::Get();

        const std::filesystem::path& pathA = *mEntries[0].TexturePath;
        if (mCachedTexA == nullptr || pathA != mCachedPathA)
        {
            mCachedTexA = texMgr.GetOrLoad(pathA);
            mCachedPathA = pathA;
        }
        const Texture* texA = mCachedTexA;
        if (!texA || !texA->IsValid())
        {
            DEBUG_LOG(sys::eLogLevel::Warning, "SkyboxRenderer: Texture A is invalid, skip draw.");
            return;
        }

        // B 側 
        const Texture* texB = texA;
        float          weight = 0.0f;

        if (mEntries.size() >= 2)
        {
            const std::filesystem::path& pathB = *mEntries[1].TexturePath;
            if (mCachedTexB == nullptr || pathB != mCachedPathB)
            {
                mCachedTexB = texMgr.GetOrLoad(pathB);
                mCachedPathB = pathB;
            }

            if (mCachedTexB && mCachedTexB->IsValid())
            {
                texB = mCachedTexB;
                weight = std::clamp(mEntries[1].Weight, 0.0f, 1.0f);
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

        // ヒープ
        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        cmdList->SetGraphicsRoot32BitConstant(
            SkyboxPipeline::SLOT_BLEND_WEIGHT,
            *reinterpret_cast<const UINT*>(&mBlendWeight),
            0);

		// GPUハンドルをセットする。SLOT_SCENE_BUFFER は t8 space0 で、FbxRenderer::mSceneBuffer の内容を参照する。
        cmdList->SetGraphicsRootDescriptorTable(
            SkyboxPipeline::SLOT_SCENE_BUFFER,
            sceneBufferGpuHandle);

        // SLOT_SKYBOX_TEX_A
        cmdList->SetGraphicsRootDescriptorTable(
            SkyboxPipeline::SLOT_SKYBOX_TEX_A,
            mTexA->GetGpuHandle());

        // SLOT_SKYBOX_TEX_B
        cmdList->SetGraphicsRootDescriptorTable(
            SkyboxPipeline::SLOT_SKYBOX_TEX_B,
            mTexB->GetGpuHandle());

        // VB / IB なし
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 0, nullptr);
        cmdList->IASetIndexBuffer(nullptr);

        // フルスクリーントライアングル
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
}