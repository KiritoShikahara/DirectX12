#include"SpriteHeader.hlsli"

// t1: sprite texture (swapped per batch in SpriteRenderer::End)
Texture2D gTexture : register(t1);
// s0: static sampler defined in RootSignature (linear filter, clamp)
SamplerState gSampler : register(s0);

// FillType に応じて FillAmount による表示クリップを行う。
// uv: TexCoord (0,0)~(1,1) 全図形・スプライト共通のUV空間。
void ApplyFillClip(float2 uv, float fillAmount, int fillType)
{
    if (fillType == 1)
    {
        // 1. Radial: clockwise circular gauge starting from 12 o'clock.
        float2 dir = uv - float2(0.5f, 0.5f);

        // atan2(x, -y) で真上を 0 とした時計回りの角度を得る。
        float angle = atan2(dir.x, -dir.y);
        if (angle < 0.0f)
            angle += 2.0f * 3.14159265f;

        float progress = angle / (2.0f * 3.14159265f);

        clip(fillAmount - progress - 0.0001f);
    }
    else
    {
        // 0. Horizontal gauge-style fill: discard pixels to the right of FillAmount.
        // TexCoord.x == 0 is the left edge, 1 is the right edge.
        // Subtract a small epsilon so FillAmount == 0 fully discards the left edge pixel too.
        clip(fillAmount - uv.x - 0.0001f);
    }
}

float4 main(VSOutput input) : SV_TARGET
{
    ApplyFillClip(input.TexCoord, input.FillAmount, input.FillType);

    float4 texColor = gTexture.Sample(gSampler, input.TexCoord);

    // Discard fully transparent pixels to avoid depth artifacts
    clip(texColor.a - 0.001f);

    // Texture color * multiply color * brightness
    float4 result = texColor * input.Color;
    result.rgb *= input.Intensity;

    return result;
}

