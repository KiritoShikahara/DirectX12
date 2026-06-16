#include "pch.h"
#include "TextPipeline.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Shader/ShaderManager.h>
#include<system/AssetPath/AssetPathManager.h>
#include <d3dx12.h>

namespace graphics
{
    bool TextPipeline::Create(DX12Device& device, ShaderManager& shaderManager)
    {
        ID3D12Device* d3d = device.GetDevice();

        // -----------------------------------------------------------------------
        //  Root Signature
        // -----------------------------------------------------------------------
        CD3DX12_DESCRIPTOR_RANGE1 cbvRange, srvRange;
        cbvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0
        srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

        CD3DX12_ROOT_PARAMETER1 params[3] = {};
        params[SLOT_SCENE_CBV].InitAsDescriptorTable(1, &cbvRange, D3D12_SHADER_VISIBILITY_ALL);
        params[SLOT_ATLAS_SRV].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
        params[SLOT_COLOR].InitAsConstants(4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0; // s0
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
        rsDesc.Init_1_1(
            _countof(params), params,
            1, &sampler,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Blob serialized, error;
        HRESULT hr = D3DX12SerializeVersionedRootSignature(
            &rsDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &serialized, &error);

        if (FAILED(hr))
        {
            if (error)
                DEBUG_LOG(sys::eLogLevel::Error, "TextPipeline: RootSignature serialize failed: {}",
                    static_cast<char*>(error->GetBufferPointer()));
            return false;
        }

        hr = d3d->CreateRootSignature(
            0,
            serialized->GetBufferPointer(),
            serialized->GetBufferSize(),
            IID_PPV_ARGS(&mRootSignature));

        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "TextPipeline: CreateRootSignature failed.");
            return false;
        }

        // -----------------------------------------------------------------------
        //  シェーダー読み込み
        // -----------------------------------------------------------------------
        auto vs = shaderManager.GetShader(ASSET_PATH("/Engine/Assets/Shader/Text/VS_Text.hlsl").string(), "main", "vs_6_0");
        auto ps = shaderManager.GetShader(ASSET_PATH("/Engine/Assets/Shader/Text/PS_Text.hlsl").string(), "main", "ps_6_0");

        if (!vs || !ps)
        {
            DEBUG_LOG(sys::eLogLevel::Error, "TextPipeline: Shader not found.");
            return false;
        }

        // -----------------------------------------------------------------------
        //  PSO
        // -----------------------------------------------------------------------
        D3D12_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // アルファブレンド (src=SA, dst=1-SA)
        D3D12_BLEND_DESC blend = {};
        blend.RenderTarget[0].BlendEnable = TRUE;
        blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        auto raster = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        raster.CullMode = D3D12_CULL_MODE_NONE;

        auto depthStencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        depthStencil.DepthEnable = FALSE; // 2D UI: 深度テスト不要

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = mRootSignature.Get();
        psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
        psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
        psoDesc.InputLayout = { layout, _countof(layout) };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.BlendState = blend;
        psoDesc.RasterizerState = raster;
        psoDesc.DepthStencilState = depthStencil;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.NumRenderTargets = 1;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;

        hr = d3d->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "TextPipeline: CreateGraphicsPipelineState failed.");
            return false;
        }

        DEBUG_LOG(sys::eLogLevel::Log, "TextPipeline: Created successfully.");
        return true;
    }

} // namespace graphics