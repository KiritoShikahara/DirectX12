struct FbxInstanceData
{
    float4x4 World;
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float Intensity;
    uint BoneOffset;
    uint BoneCount;
    float3 _pad;
};

StructuredBuffer<FbxInstanceData> gInstances : register(t0);
StructuredBuffer<float4x4> gBoneMatrices : register(t1);

// ----------------------------------------------------------
//  ì¸èoóÕç\ë¢ëÃ
// ----------------------------------------------------------

struct VSInput
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float4 Color : COLOR;
    int4 BoneIndices : BLENDINDICES;
    float4 BoneWeights : BLENDWEIGHT;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float4 VertexColor : COLOR;
    float4 BaseColor : COLOR1;
    float Intensity : TEXCOORD1;
};
