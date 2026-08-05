#include "pch.h"
#include "SkyboxPipeline.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Shader/ShaderManager.h>
#include <system/AssetPath/AssetPathManager.h>
#include <d3dx12.h>

namespace graphics
{
    bool SkyboxPipeline::Create()
    {
        ID3D12Device* d3d = DX12Device::Get().GetDevice();
        if (!CreateRootSignature(d3d)) return false;
        if (!CreatePipeline(d3d)) return false;

        DEBUG_LOG(sys::eLogLevel::Log, "SkyboxPipeline: Created successfully.");
        return true;
    }

    bool SkyboxPipeline::CreateRootSignature(ID3D12Device* device)
    {
        CD3DX12_DESCRIPTOR_RANGE1 rangeScene, rangeCubeA, rangeCubeB;

        rangeScene.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0); // t8 space0

        rangeCubeA.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1); // t0 space1
        rangeCubeB.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1); // t1 space1

        CD3DX12_ROOT_PARAMETER1 params[4];

        params[SLOT_BLEND_WEIGHT].InitAsConstants(
            1,
            /*shaderRegister=*/0,
            /*registerSpace=*/0,
            D3D12_SHADER_VISIBILITY_PIXEL);

        params[SLOT_SCENE_BUFFER].InitAsDescriptorTable(
            1, &rangeScene,
            D3D12_SHADER_VISIBILITY_VERTEX);

        params[SLOT_SKYBOX_TEX_A].InitAsDescriptorTable(
            1, &rangeCubeA,
            D3D12_SHADER_VISIBILITY_PIXEL);

        params[SLOT_SKYBOX_TEX_B].InitAsDescriptorTable(
            1, &rangeCubeB,
            D3D12_SHADER_VISIBILITY_PIXEL);

        // サンプラー
        CD3DX12_STATIC_SAMPLER_DESC sampler(
            0,
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(
            _countof(params), params,
            1, &sampler,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // 生成
        Blob sigBlob, errBlob;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &sigBlob, &errBlob);
        if (FAILED(hr))
        {
            if (errBlob) OutputDebugStringA(static_cast<char*>(errBlob->GetBufferPointer()));
            DEBUG_LOG(sys::eLogLevel::Fatal, "SkyboxPipeline: Failed to serialize root signature.");
            return false;
        }

        hr = device->CreateRootSignature(
            0,
            sigBlob->GetBufferPointer(),
            sigBlob->GetBufferSize(),
            IID_PPV_ARGS(&mRootSignature));

        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "SkyboxPipeline: Failed to create root signature.");
            return false;
        }

        mRootSignature->SetName(L"SkyboxRootSignature");
        return true;
    }

    bool SkyboxPipeline::CreatePipeline(ID3D12Device* device)
    {
        auto& shaderManager = ShaderManager::Get();
        auto VS = shaderManager.GetShader(
            ASSET_PATH_UTF8("/Engine/Assets/Shader/Skybox/SkyboxVS.hlsl"), "main", "vs_6_0");
        auto PS = shaderManager.GetShader(
            ASSET_PATH_UTF8("/Engine/Assets/Shader/Skybox/SkyboxPS.hlsl"), "main", "ps_6_0");

        if (!VS || !PS)
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "SkyboxPipeline: Failed to load shaders.");
            return false;
        }

        // ラスタライザ
        auto rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        rasterDesc.CullMode = D3D12_CULL_MODE_NONE;

        // 深度
        auto dsDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = mRootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(VS.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(PS.Get());
        psoDesc.InputLayout = { nullptr, 0 }; // VB なし
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState = rasterDesc;
        psoDesc.DepthStencilState = dsDesc;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "SkyboxPipeline: Failed to create PSO.");
            return false;
        }

        mPipelineState->SetName(L"SkyboxPipelineState");
        return true;
    }

} // namespace graphics