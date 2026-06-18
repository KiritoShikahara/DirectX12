#include"pch.h"
#include "ShapePipeline.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Shader/ShaderManager.h>
#include<system/AssetPath/AssetPathManager.h>

namespace graphics
{
	D3D12_DEPTH_STENCIL_DESC ShapePipeline::MakeDepthStencilDesc()
	{
		// 2D shapes never need depth test/write.
		D3D12_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = FALSE;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.StencilEnable = FALSE;
		desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.BackFace = desc.FrontFace;
		return desc;
	}
	D3D12_BLEND_DESC ShapePipeline::MakeBlendDesc()
	{
		// Standard alpha blending (needed for circle/triangle AA edges and FillAmount cutoff).
		D3D12_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;

		D3D12_RENDER_TARGET_BLEND_DESC rt = {};
		rt.BlendEnable = TRUE;
		rt.LogicOpEnable = FALSE;
		rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rt.LogicOp = D3D12_LOGIC_OP_NOOP;
		rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		for (auto& target : desc.RenderTarget) target = rt;
		return desc;
	}
	D3D12_RASTERIZER_DESC ShapePipeline::MakeRasterizerDesc()
	{
		// Two-sided draw (supports Flip-based mirroring), no depth bias.
		D3D12_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D12_FILL_MODE_SOLID;
		desc.CullMode = D3D12_CULL_MODE_NONE;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = TRUE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		return desc;
	}

	ID3D12RootSignature* ShapePipeline::GetRootSignature() const
	{
		return mRootSignature.Get();
	}

	ID3D12PipelineState* ShapePipeline::GetPipelineState() const
	{
		return mPipelineState.Get();
	}

	/// <summary>
	/// CreateRootSignature
	/// Only one descriptor table (t0: instance StructuredBuffer) is needed,
	/// since shapes have no texture.
	/// </summary>
	bool ShapePipeline::CreateRootSignature(ID3D12Device* device)
	{
		CD3DX12_DESCRIPTOR_RANGE1 rangeBuffer;
		rangeBuffer.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

		CD3DX12_ROOT_PARAMETER1 rootParams[1];
		rootParams[0].InitAsDescriptorTable(1, &rangeBuffer, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
		rootSigDesc.Init_1_1(
			_countof(rootParams), rootParams,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Blob signatureBlob, errorBlob;
		HRESULT hr = D3D12SerializeVersionedRootSignature(
			&rootSigDesc, &signatureBlob, &errorBlob);

		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Fatal, "Failed to serialize root signature");
			return false;
		}

		hr = device->CreateRootSignature(
			0,
			signatureBlob->GetBufferPointer(),
			signatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&mRootSignature));

		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Fatal, "Failed to create root signature");
			return false;
		}

		return true;
	}

	/// <summary>
	/// CreatePipeline
	/// </summary>
	bool ShapePipeline::CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager)
	{
		auto VS = shaderManager.GetShader(ASSET_PATH("/Engine/Assets/Shader/Shape/VS_Shape.hlsl").string(), "main", "vs_6_0");
		auto PS = shaderManager.GetShader(ASSET_PATH("/Engine/Assets/Shader/Shape/PS_Shape.hlsl").string(), "main", "ps_6_0");

		if (!VS || !PS)
		{
			DEBUG_LOG(sys::eLogLevel::Fatal, "Failed to load shaders for shape pipeline");
			return false;
		}

		// Vertex layout: same unit-quad layout as Sprite.
		D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// PSO setup
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(VS.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(PS.Get());
		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.BlendState = MakeBlendDesc();
		psoDesc.RasterizerState = MakeRasterizerDesc();
		psoDesc.DepthStencilState = MakeDepthStencilDesc();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		const HRESULT hr = device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&mPipelineState));

		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Fatal, "Failed to create graphics pipeline state for shape pipeline");
			return false;
		}

		mPipelineState->SetName(L"Ecse::ShapePipeline");

		return true;
	}

	/// <summary>
	/// Creates the root signature and PSO.
	/// </summary>
	/// <param name="device">GPU device</param>
	/// <param name="shaderManager">Shader compile/cache manager</param>
	/// <returns>true on success</returns>
	bool ShapePipeline::Create(DX12Device& device, ShaderManager& shaderManager)
	{
		ID3D12Device* d3dDevice = device.GetDevice();

		if (!CreateRootSignature(d3dDevice))
		{
			return false;
		}

		if (!CreatePipeline(d3dDevice, shaderManager))
		{
			return false;
		}

		DEBUG_LOG(sys::eLogLevel::Log, "Shape pipeline created successfully");

		return true;
	}

}