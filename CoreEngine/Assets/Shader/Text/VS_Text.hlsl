// TextVS.hlsl
// スクリーン座標 (px) → NDC 変換

cbuffer TextSceneData : register(b0)
{
    float ScreenW;
    float ScreenH;
    float PxRange;
    float Threshold;
};

struct VSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // スクリーン px → NDC  (Y 軸反転)
    output.Position.x = (input.Position.x / ScreenW) * 2.0 - 1.0;
    output.Position.y = -(input.Position.y / ScreenH) * 2.0 + 1.0;
    output.Position.z = 0.0;
    output.Position.w = 1.0;

    output.TexCoord = input.TexCoord;

    return output;
}
