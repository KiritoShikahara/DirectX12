#include "FbxShader.hlsli"

VSOutput main(VSInput input, uint instIdx : SV_InstanceID)
{
    FbxInstanceData inst = InstanceBuffer[instIdx];
    FbxSceneData scene = SceneBuffer[0];

    float3 pos = input.Position;
    float3 normal = input.Normal;
    float3 tangent = input.Tangent;

    // ── スキニング ────────────────────────────────────────────
    if (inst.BoneCount > 0u)
    {
        float3 skinnedPos = float3(0, 0, 0);
        float3 skinnedNormal = float3(0, 0, 0);
        float3 skinnedTangent = float3(0, 0, 0);

        for (int i = 0; i < 4; i++)
        {
            if (input.Weight[i] > 0.0f)
            {
                // BoneIndex は頂点ごとのボーンインデックス (モデルローカル)
                // BoneOffset はこのエンティティのボーンがBoneBuffer内で始まるオフセット
                uint boneIdx = inst.BoneOffset + (uint) max(input.BoneIndex[i], 0);
                float4x4 boneMat = BoneBuffer[boneIdx];

                skinnedPos += mul(float4(pos, 1.0f), boneMat).xyz * input.Weight[i];
                skinnedNormal += mul(normal, (float3x3) boneMat) * input.Weight[i];
                skinnedTangent += mul(tangent, (float3x3) boneMat) * input.Weight[i];
            }
        }
        pos = skinnedPos;
        normal = skinnedNormal;
        tangent = skinnedTangent;
    }

    // ── ワールド変換 ──────────────────────────────────────────
    float4 worldPos4 = mul(float4(pos, 1.0f), inst.World);

    // TBN をワールド空間へ
    float3x3 worldMat3 = (float3x3) inst.World;
    float3 N = normalize(mul(normal, worldMat3));
    float3 T = normalize(mul(tangent, worldMat3));
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt 直交化
    float3 B = cross(N, T);

    // ── 出力 ─────────────────────────────────────────────────
    VSOutput output = (VSOutput) 0;
    output.Position = mul(worldPos4, scene.ViewProjection);
    output.WorldPos = worldPos4.xyz;
    output.UV = input.UV;
    output.WorldNormal = N;
    output.WorldTangent = T;
    output.WorldBitan = B;
    output.InstIdx = instIdx;
    return output;
}
