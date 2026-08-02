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
    // MSDFはRGB各チャンネルの符号付き距離値からmedianで輪郭を再構築する特殊なエンコーディングのため、
    // 通常のミップマップ(平均化フィルタ)を経由すると距離値が壊れてガビガビしたノイズになる。
    // アンチエイリアシングはSDFの数式(screenPxRange/coverage計算)側で行うため、
    // ミップは使わずレベル0を明示的に固定してサンプリングする。
    float4 msd = AtlasTexture.SampleLevel(LinearSampler, input.TexCoord, 0);

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
