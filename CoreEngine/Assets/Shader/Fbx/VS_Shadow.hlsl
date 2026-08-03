//=============================================================================
// VS_Shadow.hlsl  -  Shadow Map 生成パス専用頂点シェーダー
//
// ・DepthOnly パス。PixelShader は不要 (PSO で null 設定)
// ・スキニングロジックは VS_Fbx.hlsl と共通
// ・LightViewProj は Root32BitConstant (b1) で渡されたライトインデックスを使い
//   LightBuffer から取得する
//=============================================================================
#include "FbxShader.hlsli"

// Shadow Pass 用ライトインデックス (Root32BitConstant / b1)
cbuffer ShadowLightIndexCB : register(b1)
{
    uint g_ShadowLightIndex;
};

// Shadow Pass 用出力 (深度書き込みのみ。SV_POSITION だけ必要)
struct VS_SHADOW_OUT
{
    float4 Position : SV_POSITION;
};

VS_SHADOW_OUT main(VSInput input)
{
    uint instanceIndex = g_InstanceBase + input.InstanceID;
    FbxInstanceData inst = InstanceBuffer[instanceIndex];
    LightData light = LightBuffer[g_ShadowLightIndex];

    float3 pos = input.Position;

    // ── スキニング ────────────────────────────────────────────
    if (inst.BoneCount > 0u)
    {
        float3 skinnedPos = float3(0, 0, 0);

        for (int i = 0; i < 4; i++)
        {
            if (input.Weight[i] > 0.0f)
            {
                uint boneIdx = inst.BoneOffset + (uint) max(input.BoneIndex[i], 0);
                float4x4 boneMat = BoneBuffer[boneIdx].Mat;
                skinnedPos += mul(float4(pos, 1.0f), boneMat).xyz * input.Weight[i];
            }
        }
        pos = skinnedPos;
    }

    // ── ライト空間へ変換 ──────────────────────────────────────
    float4 worldPos = mul(float4(pos, 1.0f), inst.World);

    VS_SHADOW_OUT output;
    output.Position = mul(worldPos, light.LightViewProj);
    return output;
}
