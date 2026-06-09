#include "pch.h"
#include "LightSystem.h"

#include<ecs/component/Light/LightComponent.h>
#include<ecs/component/transform/TransformComponent.h>
#include<graphics/Fbx/Renderer/FbxRenderer.h>

using namespace DirectX;

namespace sys
{

	/// <summary>
	/// FbxRenderer::Begin() の前に呼ぶこと
	/// TransformコンポーネントからPosition/Directionを自動取得する
	/// </summary>
	void LightSystem::Update(entt::registry& registry)
	{
        std::vector<graphics::LightData> lights;
        lights.reserve(16);

        // ディレクションライト
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

                lights.push_back(d);
            });

        // ポイントライト
        // PositionはTransformコンポーネントのワールド位置から取得
        registry.view<ecs::PointLightComponent, ecs::Transform>().each(
            [&](ecs::PointLightComponent& c, ecs::Transform& tr)
            {
                if (!c.IsActive) return;

                graphics::LightData d = {};
                d.Type = static_cast<uint32_t>(ecs::eLightType::Point);
                d.Color = c.Color;
                d.Intensity = c.Intensity;
                d.Range = c.Range;

                // ワールド行列から平行移動成分を取得
                XMMATRIX world = tr.GetWorldMatrix();
                XMStoreFloat3(&d.Position, world.r[3]);

                lights.push_back(d);
            });

        // スポットライト
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

                // DirectionalLight を持つエンティティを列挙
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

                        const std::string header = "Directional Light [" + std::to_string(static_cast<uint32_t>(entity)) + "]";
                        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            // ── Active トグル ───────────────────────────────
                            ImGui::Checkbox("Active", &light.IsActive);

                            ImGui::Separator();

                            // ── 方位角 / 仰角 スライダー ───────────────────
                            // Direction → (Azimuth, Elevation) に変換して編集
                            // Azimuth  : Y軸周り 0?360度
                            // Elevation: 水平面からの角度 -90?90度 (負 = 下向き)
                            using namespace DirectX;

                            XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&light.Direction));
                            XMFLOAT3 d;
                            XMStoreFloat3(&d, dir);

                            // 球面座標に変換
                            float elevation = XMConvertToDegrees(std::asinf(-d.y));          // 下向きを正の仰角として扱う
                            float azimuth = XMConvertToDegrees(std::atan2f(d.x, d.z));     // XZ平面の方位

                            bool dirChanged = false;
                            dirChanged |= ImGui::SliderFloat("Azimuth (deg)", &azimuth, -180.0f, 180.0f, "%.1f");
                            dirChanged |= ImGui::SliderFloat("Elevation (deg)", &elevation, -90.0f, 90.0f, "%.1f");

                            if (dirChanged)
                            {
                                // 球面座標 → 直交座標に戻す
                                const float elevRad = XMConvertToRadians(elevation);
                                const float azimRad = XMConvertToRadians(azimuth);
                                const float cosElev = std::cosf(elevRad);
                                light.Direction =
                                {
                                    cosElev * std::sinf(azimRad),
                                    -std::sinf(elevRad),           // 下向きが負Y
                                    cosElev * std::cosf(azimRad)
                                };
                            }

                            ImGui::Spacing();

                            // ── XYZ 直接編集 (参考表示 + 微調整用) ────────
                            ImGui::Text("Direction (normalized)");
                            float dir3[3] = { light.Direction.x, light.Direction.y, light.Direction.z };
                            if (ImGui::DragFloat3("##Dir", dir3, 0.01f, -1.0f, 1.0f, "%.3f"))
                            {
                                // 正規化して書き戻す
                                XMVECTOR v = XMVector3Normalize(XMLoadFloat3(
                                    reinterpret_cast<XMFLOAT3*>(dir3)));
                                XMStoreFloat3(&light.Direction, v);
                            }

                            ImGui::Separator();

                            // ── Color ───────────────────────────────────────
                            ImGui::Text("Color");
                            float col[3] = { light.Color.x, light.Color.y, light.Color.z };
                            if (ImGui::ColorEdit3("##Color", col,
                                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB))
                            {
                                light.Color = { col[0], col[1], col[2] };
                            }

                            // ── Intensity ───────────────────────────────────
                            ImGui::DragFloat("Intensity", &light.Intensity, 0.01f, 0.0f, 100.0f, "%.2f");

                            ImGui::Spacing();

                            // ── 読み取り専用情報 ────────────────────────────
                            ImGui::Separator();
                            ImGui::TextDisabled("Direction raw: (%.3f, %.3f, %.3f)",
                                light.Direction.x, light.Direction.y, light.Direction.z);
                        }

                        ImGui::PopID();
                    });

                ImGui::End();

            });
#endif // _DEBUG || DEV_TOOL_ENABLED
    }
}
