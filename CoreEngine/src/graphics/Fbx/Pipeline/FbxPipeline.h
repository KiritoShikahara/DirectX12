#pragma once

#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{

	class DX12Device;
	class ShaderManager;

	class FbxPipeline
	{
	public:
		FbxPipeline() = default;
		virtual ~FbxPipeline() = default;

		FbxPipeline(const FbxPipeline&) = delete;
		FbxPipeline& operator=(const FbxPipeline&) = delete;
		FbxPipeline(FbxPipeline&&) = delete;
		FbxPipeline& operator=(FbxPipeline&&) = delete;

		/// <summary>RootSignature と PSO を生成する</summary>
		bool Create(DX12Device& device, ShaderManager& shaderManager);

		ID3D12RootSignature* GetRootSignature() const;
		ID3D12PipelineState* GetPipelineState() const;

		// ルートパラメータインデックス（End() 内でのバインドに使用）
		static constexpr UINT SLOT_INSTANCE_BUFFER = 0;  // t0
		static constexpr UINT SLOT_BONE_BUFFER = 1;  // t1
		static constexpr UINT SLOT_DIFFUSE_TEX = 2;  // t2

	private:
		bool CreateRootSignature(ID3D12Device* device);
		bool CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager);

		static D3D12_DEPTH_STENCIL_DESC MakeDepthStencilDesc();
		static D3D12_BLEND_DESC         MakeBlendDesc();
		static D3D12_RASTERIZER_DESC    MakeRasterizerDesc();

		RootSig mRootSignature = nullptr;
		PSO     mPipelineState = nullptr;
	};
}


