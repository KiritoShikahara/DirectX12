#include"SpriteHeader.hlsli"

VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    // SV_InstanceID は DrawInstanced の StartInstanceLocation が加算済みのため
    // そのままバッファのインデックスとして使える
    SpriteShaderData data = gInstanceData[instanceID];

    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), data.WVP);
    output.TexCoord = input.TexCoord;
    output.Color = data.Color;
    output.Intensity = data.Intensity;
    output.FillAmount = data.FillAmount;
    output.FillType = data.FillType;

    return output;
}