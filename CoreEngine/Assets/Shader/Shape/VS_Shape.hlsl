#include "ShapeHeader.hlsli"

VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    ShapeShaderData data = gInstanceData[instanceID];

    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), data.WVP);
    output.TexCoord = input.TexCoord;
    output.Color = data.Color;
    output.Intensity = data.Intensity;
    output.FillAmount = data.FillAmount;
    output.ShapeType = data.ShapeType;
    output.FillType = data.FillType;

    return output;
}
