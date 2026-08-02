#include "ShapeHeader.hlsli"

// Rect: always covered; the FillAmount cutoff is applied uniformly in main().
float CoverageRect(float2 uv)
{
    return 1.0f;
}

// Circle centered at UV(0.5, 0.5) with radius 0.5.
float CoverageCircle(float2 uv)
{
    float2 centered = uv - float2(0.5f, 0.5f);
    float dist = length(centered);
    float aa = max(fwidth(dist), 1e-5f);
    return 1.0f - smoothstep(0.5f - aa, 0.5f + aa, dist);
}

// Upward-pointing equilateral triangle inscribed in the UV unit square.
// Vertices: top (0.5, 0.0), bottom-right (1.0, 1.0), bottom-left (0.0, 1.0).
float CoverageTriangle(float2 uv)
{
    float2 p0 = float2(0.5f, 0.0f);
    float2 p1 = float2(1.0f, 1.0f);
    float2 p2 = float2(0.0f, 1.0f);

    float2 e0 = p1 - p0;
    float2 n0 = normalize(float2(e0.y, -e0.x));
    float d0 = dot(uv - p0, n0);

    float2 e1 = p2 - p1;
    float2 n1 = normalize(float2(e1.y, -e1.x));
    float d1 = dot(uv - p1, n1);

    float2 e2 = p0 - p2;
    float2 n2 = normalize(float2(e2.y, -e2.x));
    float d2 = dot(uv - p2, n2);

    // Inside the triangle, all three signed distances are negative;
    // taking the max gives a single approximate SDF for the AA edge.
    float d = max(d0, max(d1, d2));
    float aa = max(fwidth(d), 1e-5f);
    return 1.0f - smoothstep(-aa, aa, d);
}

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
        // 0. Horizontal gauge-style fill, shared across all shape types:
        // pixels to the right of FillAmount are discarded.
        clip(fillAmount - uv.x - 0.0001f);
    }
}

float4 main(VSOutput input) : SV_TARGET
{
    ApplyFillClip(input.TexCoord, input.FillAmount, input.FillType);

    float coverage = 1.0f;

    if (input.ShapeType == 1)
    {
        coverage = CoverageCircle(input.TexCoord);
    }
    else if (input.ShapeType == 2)
    {
        coverage = CoverageTriangle(input.TexCoord);
    }
    else
    {
        coverage = CoverageRect(input.TexCoord);
    }

    clip(coverage - 0.001f);

    float4 result = input.Color;
    result.rgb *= input.Intensity;
    result.a *= coverage;

    return result;
}
