//=============================================================================
// VS_Model.hlsl
//  3D モデル頂点シェーダー
//
// 【行列のルール】
//   CPU 側 : XMMatrixTranspose() してから送る (DirectXMath row-major → GPU)
//   HLSL 側: デフォルト float4x4 (column-major) で受け取る
//   → Transpose の Transpose = 元の行列 として正しく解釈される
//   row_major を使うと二重転置になり崩壊する
//=============================================================================

struct ModelInstanceData
{
    float4x4 World; // ← row_major 不要。CPU 側で Transpose 済み
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float Intensity;
    uint BoneOffset;
    uint BoneCount;
    float3 _pad;
};

struct CameraData
{
    float4x4 ViewProjection; // ← 同様。CPU 側で Transpose 済み
    float3 Position;
    float _pad;
};

StructuredBuffer<ModelInstanceData> InstanceBuffer : register(t0);
StructuredBuffer<float4x4> BoneBuffer : register(t1); // ← row_major 不要
StructuredBuffer<CameraData> CameraBuffer : register(t3);

// ── 頂点入力 (ModelPipeline の InputLayout と 1:1) ─────────────────────────

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV0 : TEXCOORD0;
    float2 UV1 : TEXCOORD1;
    float3 Tangent : TANGENT;
    float3 Bitangent : BINORMAL;
    uint4 BoneIndices : BLENDINDICES; // DXGI_FORMAT_R8G8B8A8_UINT → uint4
    float4 BoneWeights : BLENDWEIGHT;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 UV0 : TEXCOORD2;
    float3 Tangent : TEXCOORD3;
    float3 Bitangent : TEXCOORD4;
    uint InstIdx : TEXCOORD5;
};

// ── メイン ────────────────────────────────────────────────────────────────

VSOutput main(VSInput input, uint instIdx : SV_InstanceID)
{
    ModelInstanceData inst = InstanceBuffer[instIdx];
    CameraData cam = CameraBuffer[0];

    float3 pos = input.Position;
    float3 normal = input.Normal;
    float3 tangent = input.Tangent;
    float3 bitangent = input.Bitangent;

    // ── スキニング ────────────────────────────────────────────────────────
    if (inst.BoneCount > 0u)
    {
        float4 skinnedPos = float4(0, 0, 0, 0);
        float4 skinnedNormal = float4(0, 0, 0, 0);
        float4 skinnedTan = float4(0, 0, 0, 0);
        float4 skinnedBitan = float4(0, 0, 0, 0);
        float totalWeight = 0.0f;

        [unroll]
        for (int b = 0; b < 4; ++b)
        {
            float w = input.BoneWeights[b];
            if (w > 0.0f)
            {
                uint boneIdx = inst.BoneOffset + input.BoneIndices[b];
                float4x4 boneMat = BoneBuffer[boneIdx]; // ← column-major で読む
                skinnedPos += w * mul(float4(pos, 1.0f), boneMat);
                skinnedNormal += w * mul(float4(normal, 0.0f), boneMat);
                skinnedTan += w * mul(float4(tangent, 0.0f), boneMat);
                skinnedBitan += w * mul(float4(bitangent, 0.0f), boneMat);
                totalWeight += w;
            }
        }

        // ウェイト合計が 0 の頂点はスキニングしない
        if (totalWeight > 0.001f)
        {
            pos = skinnedPos.xyz;
            normal = skinnedNormal.xyz;
            tangent = skinnedTan.xyz;
            bitangent = skinnedBitan.xyz;
        }
    }

    // ── ワールド変換 ─────────────────────────────────────────────────────
    float4 worldPos = mul(float4(pos, 1.0f), inst.World);
    float3 worldNormal = normalize(mul(float4(normal, 0.0f), inst.World).xyz);
    float3 worldTangent = normalize(mul(float4(tangent, 0.0f), inst.World).xyz);
    float3 worldBitangent = normalize(mul(float4(bitangent, 0.0f), inst.World).xyz);

    // ── 射影変換 ─────────────────────────────────────────────────────────
    VSOutput output;
    output.Position = mul(worldPos, cam.ViewProjection);
    output.WorldPos = worldPos.xyz;
    output.Normal = worldNormal;
    output.UV0 = input.UV0;
    output.Tangent = worldTangent;
    output.Bitangent = worldBitangent;
    output.InstIdx = instIdx;

    return output;
}
