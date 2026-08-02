#include "pch.h"
#include "GeometryGenerator.h"

#include<numbers>

namespace graphics
{
	using namespace DirectX;

    FbxVertex GeometryGenerator::MakeVertex(
        const XMFLOAT3& pos,
        const XMFLOAT2& uv,
        const XMFLOAT3& normal,
        const XMFLOAT3& tangent)
    {
        FbxVertex v = {};
        v.Position = pos;
        v.UV = uv;
        v.Normal = normal;
        v.Tangent = tangent;
        // Bone / Weight はゼロ初期化済み
        return v;
    }

    // 法線に対してほぼ垂直な接線を返す
    XMFLOAT3 GeometryGenerator::CalcTangent(const XMFLOAT3& n)
    {
        XMVECTOR nv = XMLoadFloat3(&n);
        // 上方向と法線が平行に近い場合は right を使う
        XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        XMVECTOR right = XMVectorSet(1.f, 0.f, 0.f, 0.f);
        XMVECTOR ref = (XMVectorGetX(XMVector3Dot(nv, up)) > 0.99f) ? right : up;
        XMVECTOR t = XMVector3Normalize(XMVector3Cross(ref, nv));
        XMFLOAT3 out;
        XMStoreFloat3(&out, t);
        return out;
    }


    // 四角形
    void GeometryGenerator::CreateBox(
        float w, float h, float d,
        std::vector<FbxVertex>& outV,
        std::vector<uint32_t>& outI)
    {
        outV.clear(); outI.clear();
        outV.reserve(24);
        outI.reserve(36);

        const float hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;

        // face: pos[4], normal, tangent, uvs[4]
        struct Face {
            XMFLOAT3 pos[4];
            XMFLOAT3 normal;
            XMFLOAT3 tangent;
        };

        const Face faces[6] =
        {
            // +X
            { {{ hw,-hh,-hd },{ hw, hh,-hd },{ hw, hh, hd },{ hw,-hh, hd }},
              { 1,0,0 }, { 0,0,1 } },
              // -X
              { {{-hw,-hh, hd },{-hw, hh, hd },{-hw, hh,-hd },{-hw,-hh,-hd }},
                {-1,0,0 }, { 0,0,-1 } },
                // +Y
                { {{-hw, hh,-hd },{ hw, hh,-hd },{ hw, hh, hd },{-hw, hh, hd }},
                  { 0,1,0 }, { 1,0,0 } },
                  // -Y
                  { {{-hw,-hh, hd },{ hw,-hh, hd },{ hw,-hh,-hd },{-hw,-hh,-hd }},
                    { 0,-1,0 }, { 1,0,0 } },
                    // +Z
                    { {{ hw,-hh, hd },{ hw, hh, hd },{-hw, hh, hd },{-hw,-hh, hd }},
                      { 0,0,1 }, {-1,0,0 } },
                      // -Z
                      { {{-hw,-hh,-hd },{-hw, hh,-hd },{ hw, hh,-hd },{ hw,-hh,-hd }},
                        { 0,0,-1 }, { 1,0,0 } },
        };

        const XMFLOAT2 uvs[4] = { {0,1},{0,0},{1,0},{1,1} };

        for (const auto& f : faces)
        {
            uint32_t base = static_cast<uint32_t>(outV.size());
            for (int i = 0; i < 4; ++i)
                outV.push_back(MakeVertex(f.pos[i], uvs[i], f.normal, f.tangent));

            // 2三角形 (CCW)
            outI.insert(outI.end(), {
                base + 0, base + 1, base + 2,
                base + 0, base + 2, base + 3 });
        }
    }

    // UV球
    void GeometryGenerator::CreateSphere(
        float radius,
        uint32_t slices, uint32_t stacks,
        std::vector<FbxVertex>& outV,
        std::vector<uint32_t>& outI)
    {
        outV.clear(); outI.clear();

        const float pi = std::numbers::pi_v<float>;
        const float tau = pi * 2.f;

        // 頂点生成  (stacks+1) 緯線 × (slices+1) 経線
        for (uint32_t st = 0; st <= stacks; ++st)
        {
            float phi = pi * st / stacks;           // 0 .. π
            float y = radius * std::cos(phi);
            float r = radius * std::sin(phi);

            for (uint32_t sl = 0; sl <= slices; ++sl)
            {
                float theta = tau * sl / slices;    // 0 .. 2π
                float x = r * std::cos(theta);
                float z = r * std::sin(theta);

                XMFLOAT3 pos = { x, y, z };
                XMFLOAT3 nrm = { x / radius, y / radius, z / radius };
                XMFLOAT2 uv = {
                    static_cast<float>(sl) / slices,
                    static_cast<float>(st) / stacks };

                // 接線 = dPos/dTheta 方向 (正規化)
                XMFLOAT3 tan = {
                    -std::sin(theta), 0.f, std::cos(theta) };

                outV.push_back(MakeVertex(pos, uv, nrm, tan));
            }
        }

        // インデックス生成
        uint32_t ring = slices + 1;
        for (uint32_t st = 0; st < stacks; ++st)
        {
            for (uint32_t sl = 0; sl < slices; ++sl)
            {
                uint32_t a = st * ring + sl;
                uint32_t b = a + 1;
                uint32_t c = a + ring;
                uint32_t dd = c + 1;

                outI.insert(outI.end(), { a, c, b, b, c, dd });
            }
        }
    }

    // XZ平面 Y = 0
    void GeometryGenerator::CreatePlane(
        float width, float depth,
        uint32_t divsX, uint32_t divsZ,
        std::vector<FbxVertex>& outV,
        std::vector<uint32_t>& outI)
    {
        outV.clear(); outI.clear();

        const uint32_t vx = divsX + 1;
        const uint32_t vz = divsZ + 1;
        outV.reserve(vx * vz);

        const float hw = width * 0.5f;
        const float hd = depth * 0.5f;
        const XMFLOAT3 nrm = { 0.f, 1.f, 0.f };
        const XMFLOAT3 tan = { 1.f, 0.f, 0.f };

        for (uint32_t z = 0; z < vz; ++z)
        {
            for (uint32_t x = 0; x < vx; ++x)
            {
                float px = -hw + width * x / divsX;
                float pz = -hd + depth * z / divsZ;
                XMFLOAT3 pos = { px, 0.f, pz };
                XMFLOAT2 uv = {
                    static_cast<float>(x) / divsX,
                    static_cast<float>(z) / divsZ };
                outV.push_back(MakeVertex(pos, uv, nrm, tan));
            }
        }

        for (uint32_t z = 0; z < divsZ; ++z)
        {
            for (uint32_t x = 0; x < divsX; ++x)
            {
                uint32_t a = z * vx + x;
                uint32_t b = a + 1;
                uint32_t c = a + vx;
                uint32_t dd = c + 1;
                outI.insert(outI.end(), { a, c, b, b, c, dd });
            }
        }
    }

    //  CreateCylinder
    //  上蓋・下蓋付き、上下でUV独立
    void GeometryGenerator::CreateCylinder(
        float topRadius, float bottomRadius,
        float height,
        uint32_t slices, uint32_t stacks,
        std::vector<FbxVertex>& outV,
        std::vector<uint32_t>& outI)
    {
        outV.clear(); outI.clear();

        const float pi = std::numbers::pi_v<float>;
        const float tau = pi * 2.f;
        const float hh = height * 0.5f;

        // 側面
        for (uint32_t st = 0; st <= stacks; ++st)
        {
            float t = static_cast<float>(st) / stacks; // 0(下) .. 1(上)
            float y = -hh + height * t;
            float r = bottomRadius + (topRadius - bottomRadius) * t;

            // 傾きによる法線補正 (rの変化率)
            float dr = topRadius - bottomRadius;
            float len = std::sqrt(height * height + dr * dr);
            float nrY = dr / len;           // 傾き成分
            float nrR = height / len;       // 半径方向成分

            for (uint32_t sl = 0; sl <= slices; ++sl)
            {
                float theta = tau * sl / slices;
                float cx = std::cos(theta);
                float cz = std::sin(theta);

                XMFLOAT3 pos = { r * cx, y, r * cz };
                XMFLOAT3 nrm = { nrR * cx, nrY, nrR * cz };
                XMFLOAT3 tan = { -cz, 0.f, cx };
                XMFLOAT2 uv = {
                    static_cast<float>(sl) / slices,
                    1.f - t };

                outV.push_back(MakeVertex(pos, uv, nrm, tan));
            }
        }

        uint32_t ring = slices + 1;
        for (uint32_t st = 0; st < stacks; ++st)
        {
            for (uint32_t sl = 0; sl < slices; ++sl)
            {
                uint32_t a = st * ring + sl;
                uint32_t b = a + 1;
                uint32_t c = a + ring;
                uint32_t dd = c + 1;
                outI.insert(outI.end(), { a, c, b, b, c, dd });
            }
        }

        // ---- 上蓋・下蓋の共通処理 ----
        auto AddCap = [&](float y, float radius, bool top)
            {
                uint32_t centerIdx = static_cast<uint32_t>(outV.size());
                XMFLOAT3 nrm = { 0.f, top ? 1.f : -1.f, 0.f };
                XMFLOAT3 tan = { 1.f, 0.f, 0.f };
                outV.push_back(MakeVertex({ 0.f, y, 0.f }, { 0.5f, 0.5f }, nrm, tan));

                uint32_t firstRing = static_cast<uint32_t>(outV.size());
                for (uint32_t sl = 0; sl <= slices; ++sl)
                {
                    float theta = tau * sl / slices;
                    float cx = std::cos(theta);
                    float cz = std::sin(theta);
                    XMFLOAT2 uv = { cx * 0.5f + 0.5f, cz * 0.5f + 0.5f };
                    outV.push_back(MakeVertex({ radius * cx, y, radius * cz }, uv, nrm, tan));
                }

                for (uint32_t sl = 0; sl < slices; ++sl)
                {
                    uint32_t a = firstRing + sl;
                    uint32_t b = firstRing + sl + 1;
                    if (top)
                        outI.insert(outI.end(), { centerIdx, a, b });
                    else
                        outI.insert(outI.end(), { centerIdx, b, a });
                }
            };

        AddCap(-hh, bottomRadius, false);  // 下蓋
        AddCap(hh, topRadius, true);   // 上蓋
    }

    // カプセル
	// 上半球＋下半球＋円柱
    void GeometryGenerator::CreateCapsule(
        float radius, float height,
        uint32_t slices, uint32_t hemisphereStacks,
        std::vector<FbxVertex>& outV,
        std::vector<uint32_t>& outI)
    {
        outV.clear(); outI.clear();

        const float pi = std::numbers::pi_v<float>;
        const float tau = pi * 2.f;
        const float hh = height * 0.5f;   // シリンダー半高さ

        // 上半球 (phi: 0 .. π/2)
        for (uint32_t st = 0; st <= hemisphereStacks; ++st)
        {
            float phi = (pi * 0.5f) * st / hemisphereStacks;  // 0 .. π/2
            float y = hh + radius * std::cos(phi);
            float r = radius * std::sin(phi);

            for (uint32_t sl = 0; sl <= slices; ++sl)
            {
                float theta = tau * sl / slices;
                float cx = std::cos(theta), cz = std::sin(theta);
                XMFLOAT3 pos = { r * cx, y, r * cz };
                XMFLOAT3 nrm = { r * cx / radius, (y - hh) / radius, r * cz / radius };
                XMFLOAT3 tan = { -cz, 0.f, cx };
                XMFLOAT2 uv = {
                    static_cast<float>(sl) / slices,
                    0.5f * (1.f - static_cast<float>(st) / hemisphereStacks) };
                outV.push_back(MakeVertex(pos, uv, nrm, tan));
            }
        }

        // シリンダー部分 (2リング、最小限) 
        for (uint32_t st = 0; st <= 1; ++st)
        {
            float y = hh - height * st;  // hh → -hh

            for (uint32_t sl = 0; sl <= slices; ++sl)
            {
                float theta = tau * sl / slices;
                float cx = std::cos(theta), cz = std::sin(theta);
                XMFLOAT3 pos = { radius * cx, y, radius * cz };
                XMFLOAT3 nrm = { cx, 0.f, cz };
                XMFLOAT3 tan = { -cz, 0.f, cx };
                XMFLOAT2 uv = {
                    static_cast<float>(sl) / slices,
                    0.5f + 0.25f * st };
                outV.push_back(MakeVertex(pos, uv, nrm, tan));
            }
        }

        // 下半球 (phi: π/2 .. π) 
        for (uint32_t st = 0; st <= hemisphereStacks; ++st)
        {
            float phi = (pi * 0.5f) + (pi * 0.5f) * st / hemisphereStacks; // π/2 .. π
            float y = -hh + radius * std::cos(phi);
            float r = radius * std::sin(phi);

            for (uint32_t sl = 0; sl <= slices; ++sl)
            {
                float theta = tau * sl / slices;
                float cx = std::cos(theta), cz = std::sin(theta);
                XMFLOAT3 pos = { r * cx, y, r * cz };
                XMFLOAT3 nrm = { r * cx / radius, (y + hh) / radius, r * cz / radius };
                XMFLOAT3 tan = { -cz, 0.f, cx };
                XMFLOAT2 uv = {
                    static_cast<float>(sl) / slices,
                    0.75f + 0.25f * static_cast<float>(st) / hemisphereStacks };
                outV.push_back(MakeVertex(pos, uv, nrm, tan));
            }
        }

        // インデックス生成 (連続したリング列として処理) 
        // 総リング数 = (hemisphereStacks+1) + 2 + (hemisphereStacks+1)
        uint32_t ring = slices + 1;
        uint32_t totalRings =
            (hemisphereStacks + 1) + 2 + (hemisphereStacks + 1);

        for (uint32_t r = 0; r + 1 < totalRings; ++r)
        {
            for (uint32_t sl = 0; sl < slices; ++sl)
            {
                uint32_t a = r * ring + sl;
                uint32_t b = a + 1;
                uint32_t c = a + ring;
                uint32_t dd = c + 1;
                outI.insert(outI.end(), { a, c, b, b, c, dd });
            }
        }
    }

}