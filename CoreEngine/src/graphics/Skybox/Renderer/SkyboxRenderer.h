#pragma once

#include<Utility/Singleton/Singleton.hpp>

#include<memory>
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
        std::unique_ptr<SkyboxPipeline> mPipeline;
        GDescriptorHeapManager* mHeapManager = nullptr;

        // UpdateAndDraw() → End() へ渡す描画パラメータ
        const Texture* mTexA = nullptr;
        const Texture* mTexB = nullptr;
        float          mBlendWeight = 0.0f;
        bool           mHasDraw = false;
	};
}


