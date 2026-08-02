cbuffer SkyboxCB : register(b0, space0)
{
    float gBlendWeight;
};

// テクスチャ A/B (space1 で FBX の space0 と衝突回避)
TextureCube<float4> gTexA : register(t0, space1);
TextureCube<float4> gTexB : register(t1, space1);

SamplerState gSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float3 WorldDir : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // VS の補間後に歪みが生じるため PS 側で再正規化
    float3 dir = normalize(input.WorldDir);

    float4 colorA = gTexA.Sample(gSampler, dir);
    float4 colorB = gTexB.Sample(gSampler, dir);

    // gBlendWeight = 0 → A のみ / 1 → B のみ
    return lerp(colorA, colorB, gBlendWeight);
}