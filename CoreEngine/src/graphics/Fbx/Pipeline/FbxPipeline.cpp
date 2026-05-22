#include"pch.h"
#include "FbxPipeline.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Shader/ShaderManager.h>
#include<system/AssetPath/AssetPathManager.h>

namespace graphics
{
	ID3D12RootSignature* FbxPipeline::GetRootSignature() const { return mRootSignature.Get(); }
	ID3D12PipelineState* FbxPipeline::GetPipelineState() const { return mPipelineState.Get(); }

	// ステート記述ヘルパー

    D3D12_DEPTH_STENCIL_DESC FbxPipeline::MakeDepthStencilDesc()
    {
        // 3D モデルは深度テスト有効・書き込み有効
        D3D12_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
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

    D3D12_BLEND_DESC FbxPipeline::MakeBlendDesc()
    {
        // アルファブレンド（スプライトと同じ通常合成）
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

    D3D12_RASTERIZER_DESC FbxPipeline::MakeRasterizerDesc()
    {
        // 背面カリング有効（通常の 3D メッシュ）
        D3D12_RASTERIZER_DESC desc = {};
        desc.FillMode = D3D12_FILL_MODE_SOLID;
        desc.CullMode = D3D12_CULL_MODE_BACK;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthBias = 0;
        desc.DepthBiasClamp = 0.f;
        desc.SlopeScaledDepthBias = 0.f;
        desc.DepthClipEnable = TRUE;
        desc.MultisampleEnable = FALSE;
        desc.AntialiasedLineEnable = FALSE;
        desc.ForcedSampleCount = 0;
        desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        return desc;
    }

    /// <summary>
    /// ルートシグネチャの作成
    /// </summary>
    bool FbxPipeline::CreateRootSignature(ID3D12Device* device)
    {
        // t0: StructuredBuffer<FbxInstanceData>
        CD3DX12_DESCRIPTOR_RANGE1 rangeInstance;
        rangeInstance.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        // t1: StructuredBuffer<float4x4>
        CD3DX12_DESCRIPTOR_RANGE1 rangeBone;
        rangeBone.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

        // t2: Texture2D
        CD3DX12_DESCRIPTOR_RANGE1 rangeTex;
        rangeTex.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

        CD3DX12_ROOT_PARAMETER1 params[3];
        params[SLOT_INSTANCE_BUFFER].InitAsDescriptorTable(1, &rangeInstance, D3D12_SHADER_VISIBILITY_ALL);
        params[SLOT_BONE_BUFFER].InitAsDescriptorTable(1, &rangeBone, D3D12_SHADER_VISIBILITY_VERTEX);
        params[SLOT_DIFFUSE_TEX].InitAsDescriptorTable(1, &rangeTex, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC sampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(
            _countof(params), params,
            1, &sampler,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Blob sigBlob, errBlob;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &sigBlob, &errBlob);
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "FbxPipeline: Failed to serialize root signature.");
            return false;
        }

        hr = device->CreateRootSignature(
            0,
            sigBlob->GetBufferPointer(),
            sigBlob->GetBufferSize(),
            IID_PPV_ARGS(&mRootSignature));

        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "FbxPipeline: Failed to create root signature.");
            return false;
        }

        return true;
    }

    /// <summary>
	/// パイプラインステートの作成
    /// </summary>
    bool FbxPipeline::CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager)
    {
        auto VS = shaderManager.GetShader(
            ASSET_PATH("/Engine/Assets/Shader/Fbx/VS_Fbx.hlsl").string(), "main", "vs_6_0");
        auto PS = shaderManager.GetShader(
            ASSET_PATH("/Engine/Assets/Shader/Fbx/PS_Fbx.hlsl").string(), "main", "ps_6_0");

        if (!VS || !PS)
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "FbxPipeline: Failed to load shaders.");
            return false;
        }

        // FbxVertex と 1:1 対応する入力レイアウト
        D3D12_INPUT_ELEMENT_DESC inputLayout[] =
        {
            { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT,  0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 76, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

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
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        const HRESULT hr = device->CreateGraphicsPipelineState(
            &psoDesc, IID_PPV_ARGS(&mPipelineState));

        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "FbxPipeline: Failed to create pipeline state.");
            return false;
        }

        mPipelineState->SetName(L"FbxPipeline");
        return true;
    }

    /// <summary>
	/// RootSignature と PSO を生成する
    /// </summary>
    bool FbxPipeline::Create(DX12Device& device, ShaderManager& shaderManager)
    {
        ID3D12Device* d3d = device.GetDevice();
        if (!CreateRootSignature(d3d)) return false;
        if (!CreatePipeline(d3d, shaderManager)) return false;

        DEBUG_LOG(sys::eLogLevel::Log, "FbxPipeline: Created successfully.");
        return true;
    }
}