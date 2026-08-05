#include "pch.h"
#include "LinePipeline.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Shader/ShaderManager.h>

namespace graphics
{
	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	/// <returns>true:成功</returns>
	bool LinePipeline::CreateRootSignature(ID3D12Device* device)
	{
		CD3DX12_DESCRIPTOR_RANGE1 rangeCamera;
		rangeCamera.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		
		CD3DX12_ROOT_PARAMETER1 params[1];
		params[SLOT_CAMERA_BUFFER].InitAsDescriptorTable(
			1, &rangeCamera, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
		rsDesc.Init_1_1(
			_countof(params), params,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

		graphics::Blob rsBlob, errBlob;
		HRESULT hr = D3D12SerializeVersionedRootSignature(&rsDesc, &rsBlob, &errBlob);
		if (FAILED(hr))
		{
			if (errBlob)
			{
				DEBUG_LOG(sys::eLogLevel::Error,
					static_cast<const char*>(errBlob->GetBufferPointer()));
			}
			return false;
		}

		hr = device->CreateRootSignature(
			0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&mRootSignature));
		if (FAILED(hr)) return false;

		mRootSignature->SetName(L"PhysicsLinePipeline_RootSignature");
		return true;
	}

	/// <summary>
	/// 描画パイプラインの作成
	/// </summary>
	/// <returns></returns>
	bool LinePipeline::CreatePipeline(ID3D12Device* device)
	{
		// シェーダー
		auto& shaderManager = graphics::ShaderManager::Get();
		auto VS = shaderManager.GetShader(
			ASSET_PATH_UTF8("/Engine/Assets/Shader/PhysicsDebug/VS_PhysicsDebug.hlsl"),
			"main", "vs_6_0");
		auto PS = shaderManager.GetShader(
			ASSET_PATH_UTF8("/Engine/Assets/Shader/PhysicsDebug/PS_PhysicsDebug.hlsl"),
			"main", "ps_6_0");

		if (!VS || !PS)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"PhysicsLinePipeline: Failed to compile shaders.");
			return false;
		}

		// 入力レイアウト
		D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(VS.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(PS.Get());
		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

		// ブレンド
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

		// ラスタライザ
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = FALSE;
		psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;

		// 深度
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		const HRESULT hr = device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&mPipelineState));
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"PhysicsLinePipeline: Failed to create PSO.");
			return false;
		}

		mPipelineState->SetName(L"PhysicsLinePipeline_PSO");
		return true;
	}

	LinePipeline::LinePipeline()
		:mRootSignature(nullptr)
		,mPipelineState(nullptr)
	{
	}

	/// <summary>
	/// 作成
	/// </summary>
	bool LinePipeline::Initialize()
	{
		auto& device = graphics::DX12Device::Get();

		auto d3d = device.GetDevice();

		if (CreateRootSignature(d3d) == false)
		{
			return false;
		}

		if (CreatePipeline(d3d) == false)
		{
			return false;
		}

		return true;
	}
}