#include"SpriteHeader.hlsli"

// t1 : スプライトテクスチャ（バッチごとに差し替えられる）
Texture2D gTexture : register(t1);
// s0 : 静的サンプラー（RootSignature 側で定義済み）
SamplerState gSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    // テクスチャサンプリング
    float4 texColor = gTexture.Sample(gSampler, input.TexCoord);

    // アルファが極めて小さいピクセルを早期破棄
    // （半透明境界のアーティファクト・デプスバッファへの誤書き込みを防ぐ）
    clip(texColor.a - 0.001f);

    // 乗算カラー × テクスチャカラー × 輝度
    float4 result = texColor * input.Color;
    result.rgb *= input.Intensity;

    return result;
}