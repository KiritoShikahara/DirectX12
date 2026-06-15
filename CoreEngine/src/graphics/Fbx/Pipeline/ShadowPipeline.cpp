#include "pch.h"
#include "ShadowPipeline.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Shader/ShaderManager.h>
#include <system/AssetPath/AssetPathManager.h>
#include <d3dx12.h>

namespace graphics
{
    bool ShadowPipeline::Create(
        DX12Device& device,
        ShaderManager& shaderManager,
        ID3D12RootSignature* sharedRootSignature)
    {
        if (sharedRootSignature == nullptr)
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "ShadowPipeline: RootSignature is null.");
            return false;
        }

        auto VS = shaderManager.GetShader(
            ASSET_PATH("/Engine/Assets/Shader/FBX/VS_Shadow.hlsl").string(), "main", "vs_6_0");

        if (!VS)
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "ShadowPipeline: Failed to load VS_Shadow.hlsl.");
            return false;
        }

        // FbxVertex のバイナリレイアウトと完全一致 (stride = 76 bytes)
        D3D12_INPUT_ELEMENT_DESC inputLayout[] =
        {
            { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BONE_INDEX", 0, DXGI_FORMAT_R32G32B32A32_SINT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "WEIGHT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = sharedRootSignature;
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(VS.Get());
        // PS は null → DepthOnly パス
        psoDesc.PS = {};
        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };

        // ── ラスタライザ ─────────────────────────────────────────
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; // 裏面をカリングしてピーターパン軽減
        // DepthBias: 定数バイアス + スロープスケールバイアスでセルフシャドウを除去
        // 値はシャドウマップ解像度 (2048) と near/far に合わせて調整
        psoDesc.RasterizerState.DepthBias = 1000;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 1.5f;

        // ── Blend / Depth ────────────────────────────────────────
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        // ── RenderTarget なし / Depth のみ ───────────────────────
        psoDesc.NumRenderTargets = 0;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device.GetDevice()->CreateGraphicsPipelineState(
            &psoDesc, IID_PPV_ARGS(&mPipelineState));

        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "ShadowPipeline: Failed to create PSO. hr=0x{:08X}", hr);
            return false;
        }

        mPipelineState->SetName(L"ShadowPipelineState");
        DEBUG_LOG(sys::eLogLevel::Log, "ShadowPipeline: Created successfully.");
        return true;
    }

} // namespace graphics
