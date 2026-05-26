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
    FbxInstanceData  inst = gInstances[instanceID];
    CameraShaderData cam  = gCamera[0]; // ★ カメラデータ取得

    float4x4 skinMat      = CalcSkinMatrix(
        input.BoneIndices, input.BoneWeights,
        inst.BoneOffset, inst.BoneCount);

    float4 skinnedPos     = mul(float4(input.Position, 1.f), skinMat);
    float3 skinnedNormal  = normalize(mul(input.Normal,  (float3x3)skinMat));

    float4 worldPos4  = mul(skinnedPos,    inst.World);
    float3 worldNorm  = normalize(mul(skinnedNormal, (float3x3)inst.World));

    // ★ VP を適用してクリップ空間へ
    float4 clipPos    = mul(worldPos4, cam.ViewProjection);

    VSOutput o;
    o.Position    = clipPos;
    o.UV          = input.UV;
    o.Normal = worldNorm;
    o.Tangent = worldPos4.xyz;
    o.VertexColor = input.Color;
    o.BaseColor   = inst.BaseColor;
    o.Intensity   = inst.Intensity;
    return o;
}