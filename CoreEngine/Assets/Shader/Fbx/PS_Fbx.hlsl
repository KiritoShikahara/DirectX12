#include "FbxShader.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
    // Root32BitConstant から取得したインデックスで正確に参照 (VSと同じ値)
    FbxInstanceData inst = InstanceBuffer[g_InstanceIndex];
    FbxSceneData scene = SceneBuffer[0];

    // ============================================================
    //  1. マテリアルパラメータ
    // ============================================================

    // Albedo
    float4 albedoSample = inst.HasAlbedo
        ? AlbedoTexture.Sample(LinearSampler, input.UV)
        : float4(1.0f, 1.0f, 1.0f, 1.0f);
    float3 albedo = albedoSample.rgb * inst.BaseColorFactor * inst.CustomColor.rgb;
    float alpha = albedoSample.a * inst.CustomColor.a;

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
    //  2. 法線
    // ============================================================
    float3 N;
    if (inst.HasNormal)
    {
        float3 normalTS = NormalTexture.Sample(LinearSampler, input.UV).xyz * 2.0f - 1.0f;
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
    //  3. Cook-Torrance PBR (マルチライト + シャドウ)
    // ============================================================
    float3 V = normalize(scene.CameraPosition - input.WorldPos);
    float NdotV = saturate(dot(N, V));
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < scene.LightCount; ++i)
    {
        LightData light = LightBuffer[i];

        float3 L = float3(0.0f, 0.0f, 0.0f);
        float attenuation = 1.0f;

        if (light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            L = normalize(-light.Direction);
        }
        else if (light.Type == LIGHT_TYPE_POINT)
        {
            float3 toLight = light.Position - input.WorldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4f);
            float ratio = saturate(1.0f - (dist / max(light.Range, 1e-4f)));
            attenuation = ratio * ratio;
        }
        else if (light.Type == LIGHT_TYPE_SPOT)
        {
            float3 toLight = light.Position - input.WorldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4f);
            float ratio = saturate(1.0f - (dist / max(light.Range, 1e-4f)));
            attenuation = ratio * ratio;
            float cosAngle = dot(-L, normalize(light.Direction));
            float spotFactor = saturate(
                (cosAngle - light.OuterCosine) /
                max(light.InnerCosine - light.OuterCosine, 1e-4f));
            attenuation *= spotFactor;
        }

        float3 H = normalize(V + L);
        float NdotL = saturate(dot(N, L));
        float NdotH = saturate(dot(N, H));
        float HdotV = saturate(dot(H, V));

        float D = D_GGX(NdotH, roughness);
        float G = G_Smith(NdotV, NdotL, roughness);
        float3 F = F_Schlick(HdotV, F0);
        float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 1e-4f);

        float3 kD = (1.0f - F) * (1.0f - metallic);
        float3 diffuse = kD * albedo / PI;

        float3 radiance = light.Color * light.Intensity * attenuation;

        // ── Shadow ───────────────────────────────────────────
        // index 0 の Directional Light のみ Shadow Map 参照
        // (将来的に複数ライト対応する場合はここを拡張)
        float shadowFactor = 1.0f;
        if (i == 0 && light.Type == LIGHT_TYPE_DIRECTIONAL && light.CastShadow)
        {
            shadowFactor = SampleShadowPCF(input.ShadowPos, light.ShadowBias);
        }

        Lo += (diffuse + specular) * radiance * NdotL * shadowFactor;
    }

    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;
    float3 color = ambient + Lo + emissive;

    // ============================================================
    //  4. トーンマッピング (Reinhard) + ガンマ補正
    // ============================================================
    color = color / (color + 1.0f);
    color = pow(max(color, 0.0f), 1.0f / 2.2f);

    return float4(color, alpha);
}
