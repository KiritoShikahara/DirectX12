struct SpriteShaderData
{
    float4x4 WVP;
    float4 Color;
    float Intensity;
    float FillAmount;
    int FillType;
    float _pad;
};

StructuredBuffer<SpriteShaderData> gInstanceData : register(t0);

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
    int FillType : FILLTYPE;
};

