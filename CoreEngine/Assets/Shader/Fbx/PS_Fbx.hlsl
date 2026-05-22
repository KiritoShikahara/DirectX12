#include "FbxHeader.hlsli"

Texture2D gDiffuse : register(t2);
SamplerState gSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    float4 texColor = gDiffuse.Sample(gSampler, input.UV);

    // ベースカラー（マテリアル設定） × 頂点カラー × テクスチャ × 輝度
    float4 finalColor = texColor * input.BaseColor * input.VertexColor * input.Intensity;

    // アルファが極めて低い場合は破棄（半透明境界のアーティファクト抑制）
    clip(finalColor.a - 0.001f);

    return finalColor;
}
