struct ShapeShaderData
{
    float4x4 WVP;
    float4 Color;
    float Intensity;
    float FillAmount;
    int ShapeType; // 0=Rect, 1=Circle, 2=Triangle
    int FillType; // 0=Horizontal, 1=Radial. ecs::FillType と値を一致させること。
};

StructuredBuffer<ShapeShaderData> gInstanceData : register(t0);

struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
    float Intensity : INTENSITY;
    float FillAmount : FILLAMOUNT;
    int ShapeType : SHAPETYPE;
    int FillType : FILLTYPE;
};
