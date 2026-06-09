struct CameraData
{
    float4x4 ViewProjection;
};
ConstantBuffer<CameraData> gCamera : register(b0);

struct VSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), gCamera.ViewProjection);
    output.Color = input.Color;
    return output;
}