#include "pch.h"
#include "FbxPipeline.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Shader/ShaderManager.h>
#include <system/AssetPath/AssetPathManager.h>
#include <d3dx12.h>

namespace graphics
{
    // ── ルートシグネチャ ──────────────────────────────────────────

    bool FbxPipeline::CreateRootSignature(ID3D12Device* device)
    {
        // DescriptorRange (DescriptorTable スロット分)
        CD3DX12_DESCRIPTOR_RANGE1 rangeInstance, rangeBone,
            rangeAlbedo, rangeNormal,
            rangeMetallic, rangeRoughness,
            rangeAO, rangeEmissive,
            rangeScene, rangeLight,
            rangeShadowMap;

        rangeInstance.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
        rangeBone.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1
        rangeAlbedo.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // t2
        rangeNormal.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // t3
        rangeMetallic.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // t4
        rangeRoughness.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5); // t5
        rangeAO.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6); // t6
        rangeEmissive.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7); // t7
        rangeScene.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8); // t8
        rangeLight.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9); // t9
        rangeShadowMap.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10); // t10

        // params[13]
        CD3DX12_ROOT_PARAMETER1 params[13];
        params[SLOT_INSTANCE_BASE].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_ALL);  // b0
        params[SLOT_INSTANCE_BUFFER].InitAsDescriptorTable(1, &rangeInstance, D3D12_SHADER_VISIBILITY_ALL);
        params[SLOT_BONE_BUFFER].InitAsDescriptorTable(1, &rangeBone, D3D12_SHADER_VISIBILITY_VERTEX);
        params[SLOT_ALBEDO_TEX].InitAsDescriptorTable(1, &rangeAlbedo, D3D12_SHADER_VISIBILITY_PIXEL);
        params[SLOT_NORMAL_TEX].InitAsDescriptorTable(1, &rangeNormal, D3D12_SHADER_VISIBILITY_PIXEL);
        params[SLOT_METALLIC_TEX].InitAsDescriptorTable(1, &rangeMetallic, D3D12_SHADER_VISIBILITY_PIXEL);
        params[SLOT_ROUGHNESS_TEX].InitAsDescriptorTable(1, &rangeRoughness, D3D12_SHADER_VISIBILITY_PIXEL);
        params[SLOT_AO_TEX].InitAsDescriptorTable(1, &rangeAO, D3D12_SHADER_VISIBILITY_PIXEL);
        params[SLOT_EMISSIVE_TEX].InitAsDescriptorTable(1, &rangeEmissive, D3D12_SHADER_VISIBILITY_PIXEL);
        params[SLOT_SCENE_BUFFER].InitAsDescriptorTable(1, &rangeScene, D3D12_SHADER_VISIBILITY_ALL);
        params[SLOT_LIGHT_BUFFER].InitAsDescriptorTable(1, &rangeLight, D3D12_SHADER_VISIBILITY_ALL);
        params[SLOT_SHADOW_MAP].InitAsDescriptorTable(1, &rangeShadowMap, D3D12_SHADER_VISIBILITY_PIXEL);
        params[SLOT_SHADOW_LIGHT_INDEX].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b1

        // Static Sampler x2
        // s0: 通常テクスチャ用 Linear サンプラー
        // s1: PCF 用比較サンプラー
        CD3DX12_STATIC_SAMPLER_DESC samplers[2];

        samplers[0].Init(
            0, // s0
            D3D12_FILTER_ANISOTROPIC,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f,   // MipLODBias
            16);    // MaxAnisotropy: 斜め角度で見た際のテクスチャのボケを軽減

        samplers[1].Init(
            1, // s1
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0.0f,                                    // MipLODBias
            16,                                      // MaxAnisotropy
            D3D12_COMPARISON_FUNC_LESS_EQUAL);       // PCF 比較関数

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(
            _countof(params), params,
            _countof(samplers), samplers,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Blob sigBlob, errBlob;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &sigBlob, &errBlob);
        if (FAILED(hr))
        {
            if (errBlob) OutputDebugStringA(static_cast<char*>(errBlob->GetBufferPointer()));
            DEBUG_LOG(sys::eLogLevel::Fatal, "FbxPipeline: Failed to serialize root signature.");
            return false;
        }

        hr = device->CreateRootSignature(
            0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
            IID_PPV_ARGS(&mRootSignature));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "FbxPipeline: Failed to create root signature.");
            return false;
        }
        mRootSignature->SetName(L"FbxRootSignature");
        return true;
    }


    bool FbxPipeline::CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager)
    {
        auto VS = shaderManager.GetShader(
            ASSET_PATH("/Engine/Assets/Shader/FBX/VS_Fbx.hlsl").string(), "main", "vs_6_0");
        auto PS = shaderManager.GetShader(
            ASSET_PATH("/Engine/Assets/Shader/FBX/PS_Fbx.hlsl").string(), "main", "ps_6_0");

        if (!VS || !PS)
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "FbxPipeline: Failed to load shaders.");
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
        psoDesc.pRootSignature = mRootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(VS.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(PS.Get());
        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Fatal, "FbxPipeline: Failed to create PSO.");
            return false;
        }
        mPipelineState->SetName(L"FbxPipelineState");
        return true;
    }

    bool FbxPipeline::Create(DX12Device& device, ShaderManager& shaderManager)
    {
        ID3D12Device* d3d = device.GetDevice();
        if (!CreateRootSignature(d3d))           return false;
        if (!CreatePipeline(d3d, shaderManager)) return false;

        DEBUG_LOG(sys::eLogLevel::Log, "FbxPipeline: Created successfully.");
        return true;
    }

} // namespace graphics
