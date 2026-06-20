struct SpriteShaderData
{
    float4x4 WVP;
    float4 Color;
    float Intensity;
    float FillAmount;
    int FillType;
    float _pad0; // C++側 SpriteShaderData::_pad0 とオフセットを揃えるための明示パディング
    float2 UVScale; // offset 96
    float2 UVOffset; // offset 104
};

StructuredBuffer<SpriteShaderData> gInstanceData : register(t0);

// インスタンスデータの先頭オフセット。
// SV_InstanceID の StartInstanceLocation 加算がGPU依存で信頼できないため、
// CPU 側から DrawInstanced ごとに明示的に渡し、VS 側で手動加算する。
cbuffer InstanceOffsetBuffer : register(b0)
{
    uint InstanceOffset;
};

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
