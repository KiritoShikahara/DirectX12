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

// カメラデータ
struct CameraShaderData
{
    float4x4 ViewProjection; // VP 行列（転置済み）
    float3 Position;
    float _pad;
};

StructuredBuffer<FbxInstanceData> gInstances : register(t0);
StructuredBuffer<float4x4> gBoneMatrices : register(t1);
StructuredBuffer<CameraShaderData> gCamera : register(t3);

// ----------------------------------------------------------
//  入出力構造体
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
