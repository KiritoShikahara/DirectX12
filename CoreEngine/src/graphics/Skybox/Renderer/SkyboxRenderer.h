#pragma once

#include<Utility/Singleton/Singleton.hpp>

#include<memory>
#include<vector>
#include<filesystem>
#include<entt/entt.hpp>

#include"../Pipeline/SkyboxPipeline.h"

namespace graphics
{

    class GDescriptorHeapManager;
    class Texture;

	class SkyboxRenderer : public utility::Singleton<SkyboxRenderer>
	{
		SINGLETON_CLASS(SkyboxRenderer);
	public:
		SINGLETON_ACCESSOR(SkyboxRenderer);

        /// <summary>パイプラインを初期化する</summary>
        bool Initialize();

        /// <summary>リソース解放</summary>
        void Finalize();

        /// <summary>フレーム先頭処理 (現状は予約)</summary>
        void Begin();

        /// <summary>ECS レジストリから SkyboxComponent を収集して描画情報を準備する</summary>
        void UpdateAndDraw(entt::registry& registry);

        /// <summary>
        /// 収集した情報を元に GPU コマンドを発行する。
        /// FbxRenderer::End() の後に呼ぶこと。
        /// </summary>
        /// <param name="cmdList">描画先コマンドリスト</param>
        /// <param name="sceneBufferGpuHandle">FbxRenderer::mSceneBuffer の GPU ハンドル (t8)</param>
        void End(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE sceneBufferGpuHandle);
	private:
        // UpdateAndDraw() 内で収集する描画対象1件分
        struct Entry
        {
            int                          Priority = 0;
            float                        Weight = 0.0f;
            const std::filesystem::path* TexturePath = nullptr;
        };

        std::unique_ptr<SkyboxPipeline> mPipeline;
        GDescriptorHeapManager* mHeapManager = nullptr;

        // UpdateAndDraw()の一時バッファ。毎フレームclear()して再利用する
        std::vector<Entry> mEntries;

        // UpdateAndDraw()からEnd() へ渡す描画パラメータ
        const Texture* mTexA = nullptr;
        const Texture* mTexB = nullptr;
        float          mBlendWeight = 0.0f;
        bool           mHasDraw = false;

        std::filesystem::path mCachedPathA;
        const Texture*        mCachedTexA = nullptr;
        std::filesystem::path mCachedPathB;
        const Texture*        mCachedTexB = nullptr;
	};
}


