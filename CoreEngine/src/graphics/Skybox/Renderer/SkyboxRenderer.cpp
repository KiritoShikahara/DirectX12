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
        // パイプライン（ComPtr/UniquePtr）のリセット
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
        // 毎フレームのvector生成を避けるため、メンバ変数(mEntries)を使い回す
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

        // A 側 (最高優先度)。パスが前フレームと同じならGetOrLoad()自体を呼ばない
        // (absolute()によるパス解決+文字列生成コストを避けるため)
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

        // B 側 (2番目。存在しない場合は A と同じテクスチャで Weight=0)
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