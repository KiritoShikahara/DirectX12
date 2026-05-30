#pragma once

#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
	class DX12Device;
	class ShaderManager;

	class ModelPipeline
	{
	public:
		ModelPipeline() = default;
		~ModelPipeline() = default;

		ModelPipeline(const ModelPipeline&) = delete;
		ModelPipeline& operator=(const ModelPipeline&) = delete;
		ModelPipeline(ModelPipeline&&) = delete;
		ModelPipeline& operator=(ModelPipeline&&) = delete;

		/// <summary>
		/// パイプライン生成
		/// </summary>
		/// <returns>true:成功</returns>
		bool Create(DX12Device& device, ShaderManager& shaderManager);

		ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }
		ID3D12PipelineState* GetPipelineState() const { return mPipelineState.Get(); }

		static constexpr UINT SLOT_INSTANCE_BUFFER = 0;  // t0
		static constexpr UINT SLOT_BONE_BUFFER = 1;  // t1
		static constexpr UINT SLOT_DIFFUSE_TEX = 2;  // t2
		static constexpr UINT SLOT_CAMERA_BUFFER = 3;  // t3
		static constexpr UINT SLOT_NORMAL_TEX = 4;  // t4

	private:
		bool CreateRootSignature(ID3D12Device* device);
		bool CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager);

		static D3D12_DEPTH_STENCIL_DESC MakeDepthStencilDesc();
		static D3D12_BLEND_DESC         MakeBlendDesc();
		static D3D12_RASTERIZER_DESC    MakeRasterizerDesc();

	private:
		RootSig mRootSignature = nullptr;
		PSO     mPipelineState = nullptr;


	};
}


