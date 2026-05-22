#include "FbxHeader.hlsli"

float4x4 CalcSkinMatrix(int4 indices, float4 weights, uint boneOffset, uint boneCount)
{
    // ボーンなし（静止メッシュ）は恒等行列を返す
    if (boneCount == 0)
    {
        return float4x4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
    }

    float4x4 skin = (float4x4) 0;

    // 4 ウェイトのブレンド
    // ボーンインデックスが -1 の場合はウェイトが 0 のため影響なし
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        int boneIdx = indices[i];
        float w = weights[i];
        if (boneIdx >= 0 && w > 0.0f)
        {
            skin += gBoneMatrices[boneOffset + boneIdx] * w;
        }
    }

    return skin;
}

VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    FbxInstanceData inst = gInstances[instanceID];

    // スキニング行列の計算
    float4x4 skinMat = CalcSkinMatrix(
        input.BoneIndices,
        input.BoneWeights,
        inst.BoneOffset,
        inst.BoneCount);

    // スキニング適用（ボーンなしなら恒等行列なので position そのまま）
    float4 skinnedPos = mul(float4(input.Position, 1.0f), skinMat);
    float3 skinnedNormal = normalize(mul(input.Normal, (float3x3) skinMat));
    float3 skinnedTangent = normalize(mul(input.Tangent, (float3x3) skinMat));

    // ワールド空間へ変換（inst.World は転置済みなので mul(vec, mat) 形式）
    float4 worldPos = mul(skinnedPos, inst.World);

    VSOutput output;
    output.Position = worldPos;
    output.UV = input.UV;
    output.Normal = normalize(mul(skinnedNormal, (float3x3) inst.World));
    output.Tangent = normalize(mul(skinnedTangent, (float3x3) inst.World));
    output.VertexColor = input.Color;
    output.BaseColor = inst.BaseColor;
    output.Intensity = inst.Intensity;

    return output;
}