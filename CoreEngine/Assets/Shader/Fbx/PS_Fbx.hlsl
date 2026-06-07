#include "FbxShader.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
    FbxInstanceData inst = InstanceBuffer[input.InstIdx];
    FbxSceneData scene = SceneBuffer[0];

    // マテリアルパラメータ取得 
    // Albedo
    float4 albedoSample = inst.HasAlbedo
        ? AlbedoTexture.Sample(LinearSampler, input.UV)
        : float4(1.0f, 1.0f, 1.0f, 1.0f);
    float3 rawAlbedo = albedoSample.rgb * inst.BaseColorFactor;
    float3 albedo = rawAlbedo * inst.CustomColor.rgb;
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

    // 法線 (法線マップ → ワールド空間)
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
    //  3. Cook-Torrance PBR ライティング (マルチライト)
    // ============================================================
    float3 V = normalize(scene.CameraPosition - input.WorldPos);
    float NdotV = saturate(dot(N, V));

    // F0: 誘電体=0.04、金属=アルベド
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < scene.LightCount; ++i)
    {
        LightData light = LightBuffer[i];

        // ライト種別ごとに L と attenuation を計算
        float3 L = float3(0.0f, 0.0f, 0.0f);
        float attenuation = 1.0f;

        if (light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            // 平行光源: 距離減衰なし、方向は定数
            L = normalize(-light.Direction);
        }
        else if (light.Type == LIGHT_TYPE_POINT)
        {
            float3 toLight = light.Position - input.WorldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4f);

            // 逆二乗減衰 + Rangeでクランプ
            float ratio = saturate(1.0f - (dist / max(light.Range, 1e-4f)));
            attenuation = ratio * ratio;
        }
        else if (light.Type == LIGHT_TYPE_SPOT)
        {
            float3 toLight = light.Position - input.WorldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4f);

            // 距離減衰
            float ratio = saturate(1.0f - (dist / max(light.Range, 1e-4f)));
            attenuation = ratio * ratio;

            // スポット角度減衰 (Inner〜Outerの間で smooth falloff)
            float cosAngle = dot(-L, normalize(light.Direction));
            float spotFactor = saturate(
                (cosAngle - light.OuterCosine) /
                max(light.InnerCosine - light.OuterCosine, 1e-4f));
            attenuation *= spotFactor;
        }

        //BRDF 計算
        float3 H = normalize(V + L);
        float NdotL = saturate(dot(N, L));
        float NdotH = saturate(dot(N, H));
        float HdotV = saturate(dot(H, V));

        // スペキュラー (Cook-Torrance)
        float D = D_GGX(NdotH, roughness);
        float G = G_Smith(NdotV, NdotL, roughness);
        float3 F = F_Schlick(HdotV, F0);
        float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 1e-4f);

        // ディフューズ (Lambert、金属成分はゼロ)
        float3 kD = (1.0f - F) * (1.0f - metallic);
        float3 diffuse = kD * albedo / PI;

        // 放射輝度
        float3 radiance = light.Color * light.Intensity * attenuation;

        Lo += (diffuse + specular) * radiance * NdotL;
    }

    // アンビエント (IBL未実装の間は定数代替)
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;

    float3 color = ambient + Lo + emissive;

    // トーンマッピング(Reinhard) + ガンマ補正
    color = color / (color + 1.0f);
    color = pow(max(color, 0.0f), 1.0f / 2.2f);

    return float4(color, alpha);
}