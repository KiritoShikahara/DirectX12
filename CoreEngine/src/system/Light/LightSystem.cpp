#include "pch.h"
#include "LightSystem.h"

#include <ecs/component/Light/LightComponent.h>
#include <ecs/component/transform/TransformComponent.h>
#include <graphics/Fbx/Renderer/FbxRenderer.h>

using namespace DirectX;

namespace sys
{
    void LightSystem::Update(entt::registry& registry)
    {
        std::vector<graphics::LightData> lights;
        lights.reserve(16);

        // ── Directional Light ─────────────────────────────────────
        // Shadow を落とす Directional Light は必ず先頭 (index 0) に積む。
        // FbxRenderer / シェーダー側が index 0 固定で Shadow Map を参照するため。
        registry.view<ecs::DirectionalLightComponent>().each(
            [&](ecs::DirectionalLightComponent& c)
            {
                if (!c.IsActive) return;

                graphics::LightData d = {};
                d.Type = static_cast<uint32_t>(ecs::eLightType::Directional);
                d.Color = c.Color;
                d.Intensity = c.Intensity;

                XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&c.Direction));
                XMStoreFloat3(&d.Direction, dir);

                // ── Shadow Map 設定 ───────────────────────────────
                if (c.CastShadow)
                {
                    d.CastShadow = 1u;
                    d.ShadowBias = c.ShadowBias;

                    // ライト位置 = 注視点からライト方向の逆向きに ShadowDistance だけ離れた位置
                    // (ライト方向は「光が向かう方向」なので逆向きがライト座標)
                    XMVECTOR target = XMLoadFloat3(&c.ShadowTarget);
                    XMVECTOR lightPos = XMVectorSubtract(
                        target,
                        XMVectorScale(dir, c.ShadowDistance));

                    // View 行列
                    XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
                    // ライト方向がほぼ真上/真下のとき up が平行になるので回避
                    if (XMVectorGetX(XMVector3Dot(dir, up)) > 0.99f)
                        up = XMVectorSet(0.f, 0.f, 1.f, 0.f);

                    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, target, up);

                    // 正射影 Proj 行列 (Directional Light は平行光源)
                    XMMATRIX lightProj = XMMatrixOrthographicLH(
                        c.ShadowRange, c.ShadowRange,
                        c.ShadowNear, c.ShadowFar);

                    // CPU 側で転置して格納 (シェーダーは column-major で受け取る)
                    XMMATRIX lightVP = lightView * lightProj;
                    XMStoreFloat4x4(&d.LightViewProj, XMMatrixTranspose(lightVP));
                }
                else
                {
                    d.CastShadow = 0u;
                    d.ShadowBias = 0.f;
                    // LightViewProj はゼロ初期化のまま (シェーダー側で CastShadow==0 なら参照しない)
                }

                lights.push_back(d);
            });

        // ── Point Light ───────────────────────────────────────────
        registry.view<ecs::PointLightComponent, ecs::Transform>().each(
            [&](ecs::PointLightComponent& c, ecs::Transform& tr)
            {
                if (!c.IsActive) return;

                graphics::LightData d = {};
                d.Type = static_cast<uint32_t>(ecs::eLightType::Point);
                d.Color = c.Color;
                d.Intensity = c.Intensity;
                d.Range = c.Range;

                XMMATRIX world = tr.GetWorldMatrix();
                XMStoreFloat3(&d.Position, world.r[3]);

                lights.push_back(d);
            });

        // ── Spot Light ────────────────────────────────────────────
        registry.view<ecs::SpotLightComponent, ecs::Transform>().each(
            [&](ecs::SpotLightComponent& c, ecs::Transform& tr)
            {
                if (!c.IsActive) return;

                graphics::LightData d = {};
                d.Type = static_cast<uint32_t>(ecs::eLightType::Spot);
                d.Color = c.Color;
                d.Intensity = c.Intensity;
                d.Range = c.Range;
                d.InnerCosine = std::cos(c.InnerConeRad);
                d.OuterCosine = std::cos(c.OuterConeRad);

                XMMATRIX world = tr.GetWorldMatrix();
                XMStoreFloat3(&d.Position, world.r[3]);

                XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&c.Direction));
                XMStoreFloat3(&d.Direction, dir);

                lights.push_back(d);
            });

        graphics::FbxRenderer::Get().SetLights(lights);
    }

    void LightSystem::DebugUI(entt::registry& registry)
    {
#if defined(_DEBUG) || defined(DEV_TOOL_ENABLED)
        sys::ImGuiManager::Get().AddDebugUI([&registry]()
            {
                if (!ImGui::Begin("Light Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::End();
                    return;
                }

                auto view = registry.view<ecs::DirectionalLightComponent>();
                if (view.empty())
                {
                    ImGui::TextColored({ 1, 0.4f, 0.4f, 1 }, "No DirectionalLightComponent found.");
                    ImGui::End();
                    return;
                }

                int index = 0;
                view.each([&](entt::entity entity, ecs::DirectionalLightComponent& light)
                    {
                        ImGui::PushID(index++);

                        const std::string header = "Directional Light [" +
                            std::to_string(static_cast<uint32_t>(entity)) + "]";

                        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::Checkbox("Active", &light.IsActive);
                            ImGui::Separator();

                            // ── 方位角 / 仰角 ─────────────────────────────
                            XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&light.Direction));
                            XMFLOAT3 d;
                            XMStoreFloat3(&d, dir);

                            float elevation = XMConvertToDegrees(std::asinf(-d.y));
                            float azimuth = XMConvertToDegrees(std::atan2f(d.x, d.z));

                            bool dirChanged = false;
                            dirChanged |= ImGui::SliderFloat("Azimuth (deg)", &azimuth, -180.f, 180.f, "%.1f");
                            dirChanged |= ImGui::SliderFloat("Elevation (deg)", &elevation, -90.f, 90.f, "%.1f");

                            if (dirChanged)
                            {
                                const float elevRad = XMConvertToRadians(elevation);
                                const float azimRad = XMConvertToRadians(azimuth);
                                const float cosElev = std::cosf(elevRad);
                                light.Direction =
                                {
                                    cosElev * std::sinf(azimRad),
                                    -std::sinf(elevRad),
                                    cosElev * std::cosf(azimRad)
                                };
                            }

                            ImGui::Spacing();
                            ImGui::Text("Direction (normalized)");
                            float dir3[3] = { light.Direction.x, light.Direction.y, light.Direction.z };
                            if (ImGui::DragFloat3("##Dir", dir3, 0.01f, -1.f, 1.f, "%.3f"))
                            {
                                XMVECTOR v = XMVector3Normalize(
                                    XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(dir3)));
                                XMStoreFloat3(&light.Direction, v);
                            }

                            ImGui::Separator();

                            // ── Color / Intensity ─────────────────────────
                            ImGui::Text("Color");
                            float col[3] = { light.Color.x, light.Color.y, light.Color.z };
                            if (ImGui::ColorEdit3("##Color", col,
                                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB))
                                light.Color = { col[0], col[1], col[2] };

                            ImGui::DragFloat("Intensity", &light.Intensity, 0.01f, 0.f, 100.f, "%.2f");

                            ImGui::Separator();

                            // ── Shadow 設定 ───────────────────────────────
                            if (ImGui::CollapsingHeader("Shadow Settings"))
                            {
                                ImGui::Checkbox("Cast Shadow", &light.CastShadow);

                                if (light.CastShadow)
                                {
                                    ImGui::DragFloat("Shadow Range", &light.ShadowRange, 0.5f, 1.f, 500.f, "%.1f");
                                    ImGui::DragFloat("Shadow Near", &light.ShadowNear, 0.01f, 0.01f, 10.f, "%.3f");
                                    ImGui::DragFloat("Shadow Far", &light.ShadowFar, 1.f, 1.f, 1000.f, "%.1f");
                                    ImGui::DragFloat("Shadow Distance", &light.ShadowDistance, 0.5f, 1.f, 500.f, "%.1f");
                                    ImGui::DragFloat("Shadow Bias", &light.ShadowBias, 0.0001f, 0.f, 0.1f, "%.4f");

                                    float tgt[3] = { light.ShadowTarget.x, light.ShadowTarget.y, light.ShadowTarget.z };
                                    if (ImGui::DragFloat3("Shadow Target", tgt, 0.1f, -1000.f, 1000.f, "%.1f"))
                                        light.ShadowTarget = { tgt[0], tgt[1], tgt[2] };

                                    ImGui::TextDisabled("Light pos = Target - Direction * Distance");
                                }
                            }

                            ImGui::Spacing();
                            ImGui::TextDisabled("Dir raw: (%.3f, %.3f, %.3f)",
                                light.Direction.x, light.Direction.y, light.Direction.z);
                        }

                        ImGui::PopID();
                    });

                ImGui::End();
            }, "Light");
#endif
    }

} // namespace sys
