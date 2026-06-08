#include "pch.h"
#ifdef _DEBUG

#include "PhysicsDebugRenderer.h"

#include<ecs/component/collider/ColliderComponent.h>
#include <ecs/component/transform/TransformComponent.h>
#include <ecs/component/camera/CameraComponent.h>
#include <system/Camera/CameraSystem.h>
#include<system/ImGui/ImGuiManager.h>
#include <ecs/entity/EntityManager.h>

#include <numbers>

namespace sys
{

    // ==============================================================
    //  定数
    // ==============================================================
    static constexpr float kPi = static_cast<float>(std::numbers::pi);
    static constexpr float k2Pi = kPi * 2.f;

    // ==============================================================
    //  Initialize
    // ==============================================================

    void PhysicsDebugRenderer::Initialize(entt::registry& registry)
    {
        ImGuiManager::Get().AddDebugUI([this]()
            {
                ImGuiWindow();
            });
    }

    // ==============================================================
    //  ImGui デバッグウィンドウ
    // ==============================================================

    void PhysicsDebugRenderer::ImGuiWindow()
    {
        if (!ImGui::Begin("Physics Debug"))
        {
            ImGui::End();
            return;
        }

        ImGui::Checkbox("Show Colliders", &mEnabled);

        if (mEnabled)
        {
            ImGui::Separator();
            ImGui::Text("Colors");

            float boxCol[4];
            float sphereCol[4];
            float capsuleCol[4];
            float sensorCol[4];

            // ImU32 → float[4] に変換して ColorEdit に渡す
            auto ToFloat4 = [](ImU32 col, float out[4])
                {
                    out[0] = ((col >> 0) & 0xFF) / 255.f;
                    out[1] = ((col >> 8) & 0xFF) / 255.f;
                    out[2] = ((col >> 16) & 0xFF) / 255.f;
                    out[3] = ((col >> 24) & 0xFF) / 255.f;
                };
            auto FromFloat4 = [](const float col[4]) -> ImU32
                {
                    return IM_COL32(
                        static_cast<int>(col[0] * 255),
                        static_cast<int>(col[1] * 255),
                        static_cast<int>(col[2] * 255),
                        static_cast<int>(col[3] * 255));
                };

            ToFloat4(mBoxColor, boxCol);
            ToFloat4(mSphereColor, sphereCol);
            ToFloat4(mCapsuleColor, capsuleCol);
            ToFloat4(mSensorColor, sensorCol);

            if (ImGui::ColorEdit4("Box", boxCol))     mBoxColor = FromFloat4(boxCol);
            if (ImGui::ColorEdit4("Sphere", sphereCol))  mSphereColor = FromFloat4(sphereCol);
            if (ImGui::ColorEdit4("Capsule", capsuleCol)) mCapsuleColor = FromFloat4(capsuleCol);
            if (ImGui::ColorEdit4("Sensor", sensorCol))  mSensorColor = FromFloat4(sensorCol);

            ImGui::Separator();
            ImGui::SliderInt("Circle Segments", &mCircleSegments, 8, 32);
        }

        ImGui::End();
    }

    // ==============================================================
    //  Draw
    // ==============================================================

    void PhysicsDebugRenderer::Draw(entt::registry& registry)
    {
        if (!mEnabled) return;

        // ── メインカメラの VP 行列を取得 ──────────────────────────────
        auto& cameraSys = sys::CameraSystem::Get();
        if (!cameraSys.HasMainCamera()) return;

        const entt::entity camEntity = cameraSys.GetMainCameraEntity();
        const auto* cam = registry.try_get<ecs::CameraComponent>(camEntity);
        if (!cam) return;

        const DirectX::XMMATRIX viewProj = cam->GetViewProjectionMatrix();

        // ── スクリーンサイズ ──────────────────────────────────────────
        const ImGuiIO& io = ImGui::GetIO();
        const DirectX::XMFLOAT2 screenSize = { io.DisplaySize.x, io.DisplaySize.y };

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // ── 全コライダーを描画 ────────────────────────────────────────
        registry.view<ecs::ColliderComponent, ecs::Transform>().each(
            [&](entt::entity entity,
                const ecs::ColliderComponent& collider,
                const ecs::Transform& transform)
            {
                const bool isSensor = registry.all_of<ecs::SensorTagComponent>(entity);

                // センサーは黄色、それ以外は形状ごとの色
                ImU32 color;
                if (isSensor)
                {
                    color = mSensorColor;
                }
                else
                {
                    switch (collider.Shape)
                    {
                    case ecs::eColliderShape::Box:     color = mBoxColor;     break;
                    case ecs::eColliderShape::Sphere:  color = mSphereColor;  break;
                    case ecs::eColliderShape::Capsule: color = mCapsuleColor; break;
                    default:                           color = mBoxColor;     break;
                    }
                }

                const auto& pos = transform.GetPosition();
                const auto& rot = transform.GetRotation();

                switch (collider.Shape)
                {
                case ecs::eColliderShape::Box:
                    DrawBox(drawList, pos, rot, collider.HalfExtent,
                        viewProj, color, screenSize);
                    break;

                case ecs::eColliderShape::Sphere:
                    DrawSphere(drawList, pos, collider.Radius,
                        viewProj, color, screenSize);
                    break;

                case ecs::eColliderShape::Capsule:
                    DrawCapsule(drawList, pos, rot, collider.Radius, collider.HalfHeight,
                        viewProj, color, screenSize);
                    break;
                }
            });
    }

    // ==============================================================
    //  WorldToScreen
    // ==============================================================

    bool PhysicsDebugRenderer::WorldToScreen(
        const DirectX::XMFLOAT3& worldPos,
        const DirectX::XMMATRIX& viewProj,
        const DirectX::XMFLOAT2& screenSize,
        ImVec2& outScreen) const
    {
        using namespace DirectX;

        const XMVECTOR clip = XMVector4Transform(
            XMVectorSetW(XMLoadFloat3(&worldPos), 1.f), viewProj);

        const float w = XMVectorGetW(clip);
        if (w <= 0.0001f) return false;

        const float invW = 1.f / w;
        const float ndcX = XMVectorGetX(clip) * invW;
        const float ndcY = -XMVectorGetY(clip) * invW;

        outScreen.x = (ndcX + 1.f) * 0.5f * screenSize.x;
        outScreen.y = (ndcY + 1.f) * 0.5f * screenSize.y;
        return true;
    }

    // ==============================================================
    //  DrawLine3D（視錐台クリッピング対応）
    //
    //  クリップ空間（homogeneous）で線分を 6 面すべてにクリッピングしてから
    //  投影することで、端点が画面外・カメラ後方にある場合でも
    //  可視部分だけ正しく描画する。
    //
    //  各面の条件（DirectX の NDC は Z が [0,1]）:
    //    Near  :  w > 0  （w <= 0 はカメラ後方）
    //    Left  : -w <= x  →  x + w >= 0
    //    Right :  x <= w  →  w - x >= 0
    //    Bottom: -w <= y  →  y + w >= 0
    //    Top   :  y <= w  →  w - y >= 0
    //    Far   :  z <= w  →  w - z >= 0  (DirectX: z in [0,w])
    // ==============================================================

    void PhysicsDebugRenderer::DrawLine3D(
        ImDrawList* drawList,
        const DirectX::XMFLOAT3& from,
        const DirectX::XMFLOAT3& to,
        const DirectX::XMMATRIX& viewProj,
        const DirectX::XMFLOAT2& screenSize,
        ImU32                     color) const
    {
        using namespace DirectX;

        // ── ワールド → クリップ空間 ──────────────────────────────────
        XMVECTOR c0 = XMVector4Transform(XMVectorSetW(XMLoadFloat3(&from), 1.f), viewProj);
        XMVECTOR c1 = XMVector4Transform(XMVectorSetW(XMLoadFloat3(&to), 1.f), viewProj);

        // ── Cohen?Sutherland 風の Homogeneous クリッピング ───────────
        // 各面について t（線分パラメータ 0?1）の有効範囲 [tMin, tMax] を絞る
        float tMin = 0.f;
        float tMax = 1.f;

        // 面ごとに「符号付き距離」d0, d1 を計算し、
        // d = d0 + t*(d1-d0) >= 0 の範囲に tMin/tMax を更新する
        auto Clip = [&](float d0, float d1) -> bool
            {
                // d0 < 0 かつ d1 < 0 → 完全に外側
                if (d0 < 0.f && d1 < 0.f) return false;

                if (d0 < 0.f)
                {
                    // 端点 0 が外側 → tMin を交点に引き上げる
                    tMin = std::max(tMin, d0 / (d0 - d1));
                }
                else if (d1 < 0.f)
                {
                    // 端点 1 が外側 → tMax を交点に引き下げる
                    tMax = std::min(tMax, d0 / (d0 - d1));
                }
                return tMin <= tMax;
            };

        const float x0 = XMVectorGetX(c0), y0 = XMVectorGetY(c0);
        const float z0 = XMVectorGetZ(c0), w0 = XMVectorGetW(c0);
        const float x1 = XMVectorGetX(c1), y1 = XMVectorGetY(c1);
        const float z1 = XMVectorGetZ(c1), w1 = XMVectorGetW(c1);

        // 6 面すべてにクリップ（失敗したら線全体が不可視）
        if (!Clip(w0, w1)) return; // Near  (w > 0)
        if (!Clip(x0 + w0, x1 + w1)) return; // Left
        if (!Clip(w0 - x0, w1 - x1)) return; // Right
        if (!Clip(y0 + w0, y1 + w1)) return; // Bottom
        if (!Clip(w0 - y0, w1 - y1)) return; // Top
        if (!Clip(z0, z1)) return; // Near Z (DirectX: z >= 0)
        if (!Clip(w0 - z0, w1 - z1)) return; // Far

        // ── クリップ済みの端点を計算 ─────────────────────────────────
        // c = c0 + t * (c1 - c0) をクリップ空間のまま補間する
        const XMVECTOR delta = XMVectorSubtract(c1, c0);
        const XMVECTOR clipped0 = XMVectorAdd(c0, XMVectorScale(delta, tMin));
        const XMVECTOR clipped1 = XMVectorAdd(c0, XMVectorScale(delta, tMax));

        // ── クリップ空間 → スクリーン座標 ───────────────────────────
        auto ToScreen = [&](XMVECTOR clip) -> ImVec2
            {
                const float w = XMVectorGetW(clip);
                const float invW = 1.f / w;
                const float ndcX = XMVectorGetX(clip) * invW;
                const float ndcY = -XMVectorGetY(clip) * invW;
                return {
                    (ndcX + 1.f) * 0.5f * screenSize.x,
                    (ndcY + 1.f) * 0.5f * screenSize.y
                };
            };

        drawList->AddLine(ToScreen(clipped0), ToScreen(clipped1), color, 1.5f);
    }

    // ==============================================================
    //  LocalToWorld
    // ==============================================================

    DirectX::XMFLOAT3 PhysicsDebugRenderer::LocalToWorld(
        const DirectX::XMFLOAT3& localPos,
        const DirectX::XMFLOAT3& center,
        const DirectX::XMFLOAT4& rotation)
    {
        using namespace DirectX;

        const XMVECTOR rotated = XMVector3Rotate(
            XMLoadFloat3(&localPos),
            XMLoadFloat4(&rotation));

        XMFLOAT3 result;
        XMStoreFloat3(&result,
            XMVectorAdd(rotated, XMLoadFloat3(&center)));
        return result;
    }

    // ==============================================================
    //  DrawBox（12辺）
    // ==============================================================

    void PhysicsDebugRenderer::DrawBox(
        ImDrawList* drawList,
        const DirectX::XMFLOAT3& center,
        const DirectX::XMFLOAT4& rotation,
        const DirectX::XMFLOAT3& half,
        const DirectX::XMMATRIX& viewProj,
        ImU32                     color,
        const DirectX::XMFLOAT2& screenSize) const
    {
        // ローカル座標の 8 頂点
        const DirectX::XMFLOAT3 localVerts[8] =
        {
            { -half.x, -half.y, -half.z },
            {  half.x, -half.y, -half.z },
            {  half.x,  half.y, -half.z },
            { -half.x,  half.y, -half.z },
            { -half.x, -half.y,  half.z },
            {  half.x, -half.y,  half.z },
            {  half.x,  half.y,  half.z },
            { -half.x,  half.y,  half.z },
        };

        // ワールド座標に変換
        DirectX::XMFLOAT3 verts[8];
        for (int i = 0; i < 8; ++i)
        {
            verts[i] = LocalToWorld(localVerts[i], center, rotation);
        }

        // 12 辺を描画
        static constexpr int edges[12][2] =
        {
            {0,1},{1,2},{2,3},{3,0}, // 手前面
            {4,5},{5,6},{6,7},{7,4}, // 奥面
            {0,4},{1,5},{2,6},{3,7}, // 側面
        };

        for (const auto& e : edges)
        {
            DrawLine3D(drawList, verts[e[0]], verts[e[1]], viewProj, screenSize, color);
        }
    }

    // ==============================================================
    //  DrawSphere（XYZ 各軸に円）
    // ==============================================================

    void PhysicsDebugRenderer::DrawSphere(
        ImDrawList* drawList,
        const DirectX::XMFLOAT3& center,
        float                     radius,
        const DirectX::XMMATRIX& viewProj,
        ImU32                     color,
        const DirectX::XMFLOAT2& screenSize) const
    {
        const int   seg = mCircleSegments;
        const float step = k2Pi / static_cast<float>(seg);

        // XZ 平面（水平円）
        for (int i = 0; i < seg; ++i)
        {
            const float a0 = step * i;
            const float a1 = step * (i + 1);
            DrawLine3D(drawList,
                { center.x + radius * std::cos(a0), center.y, center.z + radius * std::sin(a0) },
                { center.x + radius * std::cos(a1), center.y, center.z + radius * std::sin(a1) },
                viewProj, screenSize, color);
        }

        // XY 平面（正面円）
        for (int i = 0; i < seg; ++i)
        {
            const float a0 = step * i;
            const float a1 = step * (i + 1);
            DrawLine3D(drawList,
                { center.x + radius * std::cos(a0), center.y + radius * std::sin(a0), center.z },
                { center.x + radius * std::cos(a1), center.y + radius * std::sin(a1), center.z },
                viewProj, screenSize, color);
        }

        // YZ 平面（側面円）
        for (int i = 0; i < seg; ++i)
        {
            const float a0 = step * i;
            const float a1 = step * (i + 1);
            DrawLine3D(drawList,
                { center.x, center.y + radius * std::sin(a0), center.z + radius * std::cos(a0) },
                { center.x, center.y + radius * std::sin(a1), center.z + radius * std::cos(a1) },
                viewProj, screenSize, color);
        }
    }

    // ==============================================================
    //  DrawCapsule（Y軸方向、円柱部＋上下半球）
    // ==============================================================

    void PhysicsDebugRenderer::DrawCapsule(
        ImDrawList* drawList,
        const DirectX::XMFLOAT3& center,
        const DirectX::XMFLOAT4& rotation,
        float                     radius,
        float                     halfHeight,
        const DirectX::XMMATRIX& viewProj,
        ImU32                     color,
        const DirectX::XMFLOAT2& screenSize) const
    {
        const int   seg = mCircleSegments;
        const float step = k2Pi / static_cast<float>(seg);

        // ── 上下の中心（ローカル Y 軸方向） ──────────────────────────
        const DirectX::XMFLOAT3 topCenter = LocalToWorld({ 0.f,  halfHeight, 0.f }, center, rotation);
        const DirectX::XMFLOAT3 bottomCenter = LocalToWorld({ 0.f, -halfHeight, 0.f }, center, rotation);

        // ── 円柱部分（上下の円 + 4本の縦線） ────────────────────────
        for (int i = 0; i < seg; ++i)
        {
            const float a0 = step * i;
            const float a1 = step * (i + 1);
            const float x0 = radius * std::cos(a0);
            const float z0 = radius * std::sin(a0);
            const float x1 = radius * std::cos(a1);
            const float z1 = radius * std::sin(a1);

            // 上円
            DrawLine3D(drawList,
                LocalToWorld({ x0, halfHeight, z0 }, center, rotation),
                LocalToWorld({ x1, halfHeight, z1 }, center, rotation),
                viewProj, screenSize, color);

            // 下円
            DrawLine3D(drawList,
                LocalToWorld({ x0, -halfHeight, z0 }, center, rotation),
                LocalToWorld({ x1, -halfHeight, z1 }, center, rotation),
                viewProj, screenSize, color);
        }

        // 縦線（90度刻みで4本）
        for (int i = 0; i < 4; ++i)
        {
            const float a = step * (seg / 4) * i;
            const float x = radius * std::cos(a);
            const float z = radius * std::sin(a);
            DrawLine3D(drawList,
                LocalToWorld({ x,  halfHeight, z }, center, rotation),
                LocalToWorld({ x, -halfHeight, z }, center, rotation),
                viewProj, screenSize, color);
        }

        // ── 半球部分（XZ・XY 各半円） ────────────────────────────────
        const int halfSeg = seg / 2;

        // 上半球
        for (int i = 0; i < halfSeg; ++i)
        {
            const float a0 = step * i;
            const float a1 = step * (i + 1);

            // XZ 断面（水平方向）
            DrawLine3D(drawList,
                LocalToWorld({ radius * std::cos(a0), halfHeight + radius * std::sin(a0), 0.f }, center, rotation),
                LocalToWorld({ radius * std::cos(a1), halfHeight + radius * std::sin(a1), 0.f }, center, rotation),
                viewProj, screenSize, color);

            // YZ 断面（奥行き方向）
            DrawLine3D(drawList,
                LocalToWorld({ 0.f, halfHeight + radius * std::sin(a0), radius * std::cos(a0) }, center, rotation),
                LocalToWorld({ 0.f, halfHeight + radius * std::sin(a1), radius * std::cos(a1) }, center, rotation),
                viewProj, screenSize, color);
        }

        // 下半球（Y を反転）
        for (int i = 0; i < halfSeg; ++i)
        {
            const float a0 = step * i;
            const float a1 = step * (i + 1);

            DrawLine3D(drawList,
                LocalToWorld({ radius * std::cos(a0), -halfHeight - radius * std::sin(a0), 0.f }, center, rotation),
                LocalToWorld({ radius * std::cos(a1), -halfHeight - radius * std::sin(a1), 0.f }, center, rotation),
                viewProj, screenSize, color);

            DrawLine3D(drawList,
                LocalToWorld({ 0.f, -halfHeight - radius * std::sin(a0), radius * std::cos(a0) }, center, rotation),
                LocalToWorld({ 0.f, -halfHeight - radius * std::sin(a1), radius * std::cos(a1) }, center, rotation),
                viewProj, screenSize, color);
        }
    }

} // namespace sys

#endif // _DEBUG