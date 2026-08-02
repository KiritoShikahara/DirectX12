#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
	class DX12Device;
	class ShaderManager;

	class ENGINE_API ShapePipeline
	{
	public:
		ShapePipeline() = default;
		~ShapePipeline() = default;

		ShapePipeline(const ShapePipeline&) = delete;
		ShapePipeline& operator=(const ShapePipeline&) = delete;
		ShapePipeline(ShapePipeline&&) = delete;
		ShapePipeline& operator=(ShapePipeline&&) = delete;

		/// <summary>
		/// 生成
		/// </summary>
		bool Create(DX12Device& device, ShaderManager& shaderManager);

		ID3D12RootSignature* GetRootSignature() const;
		ID3D12PipelineState* GetPipelineState() const;

	private:
		bool CreateRootSignature(ID3D12Device* device);
		bool CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager);

		// State descriptor helpers
		static D3D12_DEPTH_STENCIL_DESC  MakeDepthStencilDesc();
		static D3D12_BLEND_DESC          MakeBlendDesc();
		static D3D12_RASTERIZER_DESC     MakeRasterizerDesc();
	private:
		RootSig mRootSignature = nullptr;
		PSO     mPipelineState = nullptr;

	};

}