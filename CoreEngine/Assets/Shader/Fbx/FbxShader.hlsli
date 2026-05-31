
static const float PI = 3.14159265359f;

struct FbxInstanceData
{
    row_major float4x4 World;

    float3 BaseColorFactor;
    float MetallicFactor;

    float RoughnessFactor;
    float3 EmissiveFactor;

    uint BoneOffset;
    uint BoneCount;
    uint HasAlbedo;
    uint HasNormal;

    uint HasMetallic;
    uint HasRoughness;
    uint HasAO;
    uint HasEmissive;
};

struct FbxSceneData
{
    row_major float4x4 ViewProjection;
    float3 CameraPosition;
    float _pad0;
    float3 LightDirection;
    float LightIntensity;
    float3 LightColor;
    float _pad1;
};

struct FbxBoneMatrix
{
    row_major float4x4 Mat;
};

StructuredBuffer<FbxInstanceData> InstanceBuffer : register(t0);
StructuredBuffer<FbxBoneMatrix> BoneBuffer : register(t1);
Texture2D AlbedoTexture : register(t2);
Texture2D NormalTexture : register(t3);
Texture2D MetallicTexture : register(t4);
Texture2D RoughnessTexture : register(t5);
Texture2D AOTexture : register(t6);
Texture2D EmissiveTexture : register(t7);
StructuredBuffer<FbxSceneData> SceneBuffer : register(t8);

SamplerState LinearSampler : register(s0);

struct VSInput
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    int4 BoneIndex : BONE_INDEX;
    float4 Weight : WEIGHT;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
    float3 WorldNormal : TEXCOORD2;
    float3 WorldTangent : TEXCOORD3;
    float3 WorldBitan : TEXCOORD4;
    uint InstIdx : TEXCOORD5;
};

float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float f = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * f * f, 1e-7f);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float gv = NdotV / max(NdotV * (1.0f - k) + k, 1e-7f);
    float gl = NdotL / max(NdotL * (1.0f - k) + k, 1e-7f);
    return gv * gl;
}

float3 F_Schlick(float HdotV, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - HdotV), 5.0f);
}
