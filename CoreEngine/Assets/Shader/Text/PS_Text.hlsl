// TextPS.hlsl
// Multi-channel Signed Distance Field によるテキスト描画

cbuffer TextSceneData : register(b0)
{
    float ScreenW;
    float ScreenH;
    float PxRange;
    float Threshold;
};

cbuffer ColorData : register(b1)
{
    float4 GlyphColor;
};

Texture2D<float4> AtlasTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// MSDF のメディアン関数
float Median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float4 main(PSInput input) : SV_TARGET
{
    float4 msd = AtlasTexture.Sample(LinearSampler, input.TexCoord);

    float sd = Median(msd.r, msd.g, msd.b);

    // fwidth でスクリーンピクセルあたりのSDF変化量を推定してAAを掛ける
    float screenPxRange = PxRange * (1.0 / max(fwidth(input.TexCoord.x),
                                               fwidth(input.TexCoord.y)));

    float coverage = clamp((sd - Threshold) * screenPxRange + 0.5, 0.0, 1.0);

    float4 result = GlyphColor;
    result.a *= coverage;

    clip(result.a - 1.0 / 255.0);

    return result;
}
