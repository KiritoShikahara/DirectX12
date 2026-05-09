#include"SpriteHeader.hlsli"

// t1: sprite texture (swapped per batch in SpriteRenderer::End)
Texture2D gTexture : register(t1);
// s0: static sampler defined in RootSignature (linear filter, clamp)
SamplerState gSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.TexCoord);

    // Discard fully transparent pixels to avoid depth artifacts
    clip(texColor.a - 0.001f);

    // Texture color * multiply color * brightness
    float4 result = texColor * input.Color;
    result.rgb *= input.Intensity;

    return result;
}