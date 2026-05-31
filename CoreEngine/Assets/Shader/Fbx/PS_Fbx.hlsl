#include "FbxShader.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
    FbxInstanceData inst = InstanceBuffer[input.InstIdx];
    FbxSceneData scene = SceneBuffer[0];

    // ============================================================
    //  1. マテリアルパラメータ取得
    //     テクスチャが設定されている場合はサンプリング値 × Factor
    //     設定されていない場合は Factor のみを使用する
    // ============================================================

    // Albedo
    float4 albedoSample = inst.HasAlbedo
        ? AlbedoTexture.Sample(LinearSampler, input.UV)
        : float4(1.0f, 1.0f, 1.0f, 1.0f);
    float3 albedo = albedoSample.rgb * inst.BaseColorFactor;
    float alpha = albedoSample.a;

    // Metallic / Roughness
    float metallic = inst.HasMetallic
        ? MetallicTexture.Sample(LinearSampler, input.UV).r
        : inst.MetallicFactor;
    float roughness = inst.HasRoughness
        ? RoughnessTexture.Sample(LinearSampler, input.UV).r
        : inst.RoughnessFactor;
    roughness = clamp(roughness, 0.04f, 1.0f);

    // AO
    float ao = inst.HasAO
        ? AOTexture.Sample(LinearSampler, input.UV).r
        : 1.0f;

    // Emissive
    float3 emissive = (inst.HasEmissive
        ? EmissiveTexture.Sample(LinearSampler, input.UV).rgb
        : float3(0.0f, 0.0f, 0.0f)) + inst.EmissiveFactor;

    // ============================================================
    //  2. 法線 (法線マップ → ワールド空間)
    // ============================================================
    float3 N;
    if (inst.HasNormal)
    {
        // タンジェント空間ノーマルをデコード [-1, 1]
        float3 normalTS = NormalTexture.Sample(LinearSampler, input.UV).xyz * 2.0f - 1.0f;
        // TBN 行列 (行が T, B, N = tangent → world 変換)
        float3x3 TBN = float3x3(
            normalize(input.WorldTangent),
            normalize(input.WorldBitan),
            normalize(input.WorldNormal));
        N = normalize(mul(normalTS, TBN));
    }
    else
    {
        N = normalize(input.WorldNormal);
    }

    // ============================================================
    //  3. Cook-Torrance PBR ライティング
    // ============================================================
    float3 V = normalize(scene.CameraPosition - input.WorldPos);
    float3 L = normalize(scene.LightDirection);
    float3 H = normalize(V + L);

    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    // F0: 誘電体=0.04、金属=アルベド
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    // スペキュラー (DGF/4NdotV·NdotL)
    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    float3 F = F_Schlick(HdotV, F0);
    float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 1e-4f);

    // ディフューズ (Lambert、金属は0)
    float3 kD = (1.0f - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo / PI;

    // 最終ライティング
    float3 radiance = scene.LightColor * scene.LightIntensity;
    float3 Lo = (diffuse + specular) * radiance * NdotL;

    // アンビエント (IBL未実装の間は定数代替)
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;

    float3 color = ambient + Lo + emissive;

    // ============================================================
    //  4. トーンマッピング (Reinhard) + ガンマ補正
    // ============================================================
    color = color / (color + 1.0f);
    color = pow(max(color, 0.0f), 1.0f / 2.2f);

    return float4(color, alpha);
}
