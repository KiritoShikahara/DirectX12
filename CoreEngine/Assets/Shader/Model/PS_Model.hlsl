//=============================================================================
// PS_Model.hlsl
//  3D モデルピクセルシェーダー
//  ディフューズテクスチャ + 法線マップ + Blinn-Phong ライティング
//=============================================================================

struct ModelInstanceData
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

StructuredBuffer<ModelInstanceData> InstanceBuffer : register(t0);
Texture2D DiffuseTexture : register(t2);
Texture2D NormalTexture : register(t4);
SamplerState LinearSampler : register(s0);

// ── ピクセル入力 (VS_Model の VSOutput と一致) ─────────────────────────────

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 UV0 : TEXCOORD2;
    float3 Tangent : TEXCOORD3;
    float3 Bitangent : TEXCOORD4;
    uint InstIdx : TEXCOORD5;
};

// ── ライト定数 (簡易ハードコード, 後からCBに移行可) ─────────────────────

static const float3 LIGHT_DIR = normalize(float3(0.5f, 1.0f, -0.5f));
static const float3 LIGHT_COLOR = float3(1.0f, 0.98f, 0.95f);
static const float AMBIENT_COEFF = 0.15f;

// ── ヘルパー ─────────────────────────────────────────────────────────────

float3 UnpackNormal(float2 uv)
{
    float3 n = NormalTexture.Sample(LinearSampler, uv).xyz;
    return normalize(n * 2.0f - 1.0f);
}

// ── メイン ────────────────────────────────────────────────────────────────

float4 main(PSInput input) : SV_TARGET
{
    ModelInstanceData inst = InstanceBuffer[input.InstIdx];

    // ── テクスチャサンプリング ────────────────────────────────────────────
    float4 diffuseSample = DiffuseTexture.Sample(LinearSampler, input.UV0);

    // アルファ抜き (透明部分は描画しない)
    clip(diffuseSample.a - 0.01f);

    // ── 法線の決定 ────────────────────────────────────────────────────────
    // 法線マップテクスチャのデフォルト値 (128,128,255) を flat normal とみなす
    float3 tangentNormal = UnpackNormal(input.UV0);

    // タンジェント空間 → ワールド空間
    float3 T = normalize(input.Tangent);
    float3 B = normalize(input.Bitangent);
    float3 N = normalize(input.Normal);
    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(tangentNormal, TBN));

    // ── Blinn-Phong ライティング ─────────────────────────────────────────
    float NdotL = saturate(dot(worldNormal, LIGHT_DIR));
    float3 ambient = AMBIENT_COEFF * diffuseSample.rgb;
    float3 diffuse = NdotL * LIGHT_COLOR * diffuseSample.rgb;

    // スペキュラー (Roughness を Shininess に変換)
    float shininess = max(2.0f, (1.0f - inst.Roughness) * 128.0f);
    // ハーフベクトルは ViewDir が必要 (CameraBuffer から取得する場合は t3 を引数に追加)
    // 今回は View 方向のスペキュラーは省略し Diffuse のみで対応
    // (将来: CameraBuffer から Position を参照して Blinn-Phong specular を追加)

    float3 litColor = ambient + diffuse;

    // ── インスタンス属性の適用 ────────────────────────────────────────────
    litColor *= inst.BaseColor.rgb * inst.Intensity;
    float alpha = diffuseSample.a * inst.BaseColor.a;

    return float4(litColor, alpha);
}
