//=============================================================================
// FbxShader.hlsli  -  PBRスキンメッシュ 共通定義
//=============================================================================

static const float PI = 3.14159265359f;

// ============================================================
//  インスタンスインデックス (Root32BitConstant / b0)
//  DrawCall毎に SetGraphicsRoot32BitConstant で直接書き込む
//  → SV_InstanceID + StartInstanceLocation の挙動依存を排除
//  → 複数モデルを描画しても確実に正しいインスタンスデータを参照できる
// ============================================================
cbuffer FbxInstanceIndexCB : register(b0)
{
    uint g_InstanceIndex;
};

// ============================================================
//  StructuredBuffer 定義
//  行列: CPU側で XMMatrixTranspose() して格納、
//  シェーダーはデフォルト column-major float4x4 で受け取る
// ============================================================

struct FbxBoneMatrix
{
    float4x4 Mat; // CPU側転置済み
};

struct FbxInstanceData
{
    float4x4 World; // CPU側転置済み

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

    float4 CustomColor; // 乗算カラー (デフォルト = {1,1,1,1})
};

// ライト種別定数
static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;

struct LightData
{
    float3 Color;
    float Intensity;

    float3 Direction; // Directional / Spot (正規化済み)
    float Range; // Point / Spot

    float3 Position; // Point / Spot
    uint Type;

    float InnerCosine; // Spot: cos(InnerConeRad)
    float OuterCosine; // Spot: cos(OuterConeRad)
    float _pad0;
    float _pad1;
};

struct FbxSceneData
{
    float4x4 ViewProjection; // CPU側転置済み
    float3 CameraPosition;
    uint LightCount;
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
StructuredBuffer<LightData> LightBuffer : register(t9);

SamplerState LinearSampler : register(s0);

// ============================================================
//  頂点入出力 (InstIdx は g_InstanceIndex に統一したので不要)
// ============================================================
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
};

// ============================================================
//  PBR ヘルパー関数
// ============================================================

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
