#include "pch.h"
#include "TransitionRenderer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Shader/ShaderManager.h>

namespace graphics
{

	bool TransitionRenderer::Initialize()
	{
		auto& device = graphics::DX12Device::Get();
		auto d3d = device.GetDevice();
		if (CreateRootSignature(d3d) == false)
		{
			return false;
		}

		if (CreatePSO(d3d) == false)
		{
			return false;
		}

		DEBUG_LOG(sys::eLogLevel::Log, "TransitionRenderer: Initialized.");
		return true;
	}

	void TransitionRenderer::Draw(ID3D12GraphicsCommandList* cmdList, const graphics::Color& color)
	{
		if (color.a <= 0.0f) return;

		cmdList->SetPipelineState(mPSO.Get());
		cmdList->SetGraphicsRootSignature(mRootSignature.Get());
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// RGBAをシェーダーに渡す
		cmdList->SetGraphicsRoot32BitConstants(SLOT_COLOR, 4, &color, 0);

		// 三頂点で画面全体を覆う巨大三角形を描画
		cmdList->DrawInstanced(3, 1, 0, 0);
	}

	void TransitionRenderer::Finalize()
	{
		mPSO.Reset();
		mRootSignature.Reset();
		DEBUG_LOG(sys::eLogLevel::Log, "TransitionRenderer: Finalized.");
	}

	bool TransitionRenderer::CreateRootSignature(ID3D12Device* device)
	{
		CD3DX12_ROOT_PARAMETER1 params[1];
		params[SLOT_COLOR].InitAsConstants(
			4,
			0,
			0,
			D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
		rsDesc.Init_1_1(
			_countof(params), params,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		Blob rsBlob, errBlob;
		HRESULT hr = D3DX12SerializeVersionedRootSignature(
			&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &rsBlob, &errBlob);

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
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&mRootSignature));

		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "TransitionRenderer: CreateRootSignature failed.");
			return false;
		}

		mRootSignature->SetName(L"TransitionRenderer_RootSignature");

		return true;
	}

	bool TransitionRenderer::CreatePSO(ID3D12Device* device)
	{
		// シェーダー
		auto& shaderMgr = ShaderManager::Get();

		auto vs = shaderMgr.GetShader(
			ASSET_PATH_UTF8("/Engine/Assets/Shader/Transition/VS_Transition.hlsl"),
			"main", "vs_6_0");

		auto ps = shaderMgr.GetShader(
			ASSET_PATH_UTF8("/Engine/Assets/Shader/Transition/PS_Transition.hlsl"),
			"main", "ps_6_0");

		if (!vs || !ps)
		{
			DEBUG_LOG(sys::eLogLevel::Error, "TransitionRenderer: Shader compile failed.");
			return false;
		}

		// アルファブレンド
		D3D12_RENDER_TARGET_BLEND_DESC blendRT = {};
		blendRT.BlendEnable = TRUE;
		blendRT.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendRT.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendRT.BlendOp = D3D12_BLEND_OP_ADD;
		blendRT.SrcBlendAlpha = D3D12_BLEND_ONE;
		blendRT.DestBlendAlpha = D3D12_BLEND_ZERO;
		blendRT.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendRT.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0] = blendRT;

		// 深度ステンシル
		D3D12_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = FALSE;
		dsDesc.StencilEnable = FALSE;
		// ラスタライザ
		CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
		rastDesc.CullMode = D3D12_CULL_MODE_NONE;
		// POS
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
		psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
		psoDesc.RasterizerState = rastDesc;
		psoDesc.BlendState = blendDesc;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.InputLayout = { nullptr, 0 };     // 頂点バッファなし
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN; // 深度バッファなし
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.SampleDesc.Count = 1;

		HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "TransitionRenderer: CreateGraphicsPipelineState failed.");
			return false;
		}

		mPSO->SetName(L"TransitionRenderer_PSO");

		return true;
	}

}