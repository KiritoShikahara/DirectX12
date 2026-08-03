//=============================================================================
// FbxShader.hlsli  -  PBRスキンメッシュ 共通定義
//=============================================================================

static const float PI = 3.14159265359f;

// ============================================================
//  インスタンスバッチ先頭オフセット (Root32BitConstant / b0)
//  バッチ(同一リソース×同一セクション)毎に SetGraphicsRoot32BitConstant で書き込む。
//  実際のインスタンスインデックスは g_InstanceBase + SV_InstanceID。
//  StartInstanceLocation は常に0で描画し、その暗黙加算の挙動には依存しない。
//  SV_InstanceID 自体はDrawCall内で0起点であることがD3D12仕様で保証されるため
//  複数バッチを描画しても確実に正しいインスタンスデータを参照できる。
// ============================================================
cbuffer FbxInstanceBaseCB : register(b0)
{
    uint g_InstanceBase;
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
    uint CastShadow; // 1 = Shadow Map あり
    float ShadowBias; // セルフシャドウ除去バイアス

    float4x4 LightViewProj; // CPU側転置済み (Directional のみ有効)
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

// Shadow Map (t10): Directional Light の深度テクスチャ
Texture2D<float> ShadowMap : register(t10);

SamplerState LinearSampler : register(s0);
// PCF 比較サンプラー: ShadowMap.SampleCmpLevelZero() で使用
SamplerComparisonState ShadowSampler : register(s1);

// ============================================================
//  頂点入出力
// ============================================================
struct VSInput
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    int4 BoneIndex : BONE_INDEX;
    float4 Weight : WEIGHT;
    uint InstanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
    float3 WorldNormal : TEXCOORD2;
    float3 WorldTangent : TEXCOORD3;
    float3 WorldBitan : TEXCOORD4;
    float4 ShadowPos : TEXCOORD5; // ライト空間クリップ座標
    // PSはSV_InstanceIDを直接受け取れないため、VSで解決したインデックスを補間なしで渡す
    nointerpolation uint InstanceIndex : TEXCOORD6;
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

// ============================================================
//  PCF Shadow サンプリング (3x3 カーネル)
//
//  shadowPos : ライト空間クリップ座標 (VS で計算済み)
//  bias      : LightData.ShadowBias
//  戻り値    : 0.0(完全に影) 〜 1.0(完全に光)
// ============================================================
float SampleShadowPCF(float4 shadowPos, float bias)
{
    // クリップ → NDC → UV 変換
    float3 ndc = shadowPos.xyz / shadowPos.w;
    float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
    float depth = ndc.z - bias;

    // Shadow Map の外側は常に明るい (影なし)
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || ndc.z > 1.0f)
        return 1.0f;

    // Shadow Map のテクセルサイズ (2048x2048 固定)
    const float texelSize = 1.0f / 2048.0f;

    // 3x3 PCF カーネル
    float shadow = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += ShadowMap.SampleCmpLevelZero(ShadowSampler, uv + offset, depth);
        }
    }
    return shadow / 9.0f;
}
