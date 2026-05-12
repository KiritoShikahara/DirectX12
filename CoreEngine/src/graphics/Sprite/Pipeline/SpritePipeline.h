#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
	class DX12Device;
	class ShaderManager;

	class ENGINE_API SpritePipeline
	{
	public:
		SpritePipeline() = default;
		~SpritePipeline() = default;

		// コピー禁止・ムーブ禁止（GPU オブジェクトの単一所有）
		SpritePipeline(const SpritePipeline&) = delete;
		SpritePipeline& operator=(const SpritePipeline&) = delete;
		SpritePipeline(SpritePipeline&&) = delete;
		SpritePipeline& operator=(SpritePipeline&&) = delete;

		/// <summary>
		/// ルートシグネチャと PSO を作成する。
		/// </summary>
		/// <param name="device">GPU デバイス</param>
		/// <param name="shaderManager">シェーダーのコンパイル・キャッシュ管理</param>
		/// <returns>true:成功</returns>
		bool Create(DX12Device& device, ShaderManager& shaderManager);

		ID3D12RootSignature* GetRootSignature() const;
		ID3D12PipelineState* GetPipelineState() const;

	private:
		bool CreateRootSignature(ID3D12Device* device);
		bool CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager);

		// ステート記述子の生成ヘルパー
		static D3D12_DEPTH_STENCIL_DESC  MakeDepthStencilDesc();
		static D3D12_BLEND_DESC          MakeBlendDesc();
		static D3D12_RASTERIZER_DESC     MakeRasterizerDesc();
	private:
		RootSig mRootSignature = nullptr;
		PSO     mPipelineState = nullptr;

	};

}


