#pragma once
#include <DirectXMath.h>
#include <cstdint>

namespace ecs
{
    enum class eLightType : uint32_t
    {
        Directional = 0,
        Point = 1,
        Spot = 2,
    };

    struct DirectionalLightComponent
    {
        DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
        DirectX::XMFLOAT3 Color = { 1.0f,  1.0f, 1.0f };
        float             Intensity = 1.0f;
        bool              IsActive = true;

        // ── Shadow Map 設定 ───────────────────────────────────
        /// <summary>true にすると Shadow Map を生成してシャドウを落とす</summary>
        bool  CastShadow = false;

        /// <summary>
        /// 正射影の範囲 (幅・高さ共通)。
        /// シャドウを受けたいオブジェクトが収まるサイズに合わせる。
        /// 例: 50.0f → 50x50 ユニットのエリアをカバー
        /// </summary>
        float ShadowRange = 50.0f;

        /// <summary>ライトカメラの near / far クリップ</summary>
        float ShadowNear = 0.1f;
        float ShadowFar = 200.0f;

        /// <summary>
        /// ライトカメラの注視点 (ワールド空間)。
        /// シャドウを落としたいシーンの中心を指定する。
        /// </summary>
        DirectX::XMFLOAT3 ShadowTarget = { 0.0f, 0.0f, 0.0f };

        /// <summary>
        /// ライト位置をターゲットからどれだけ離すか。
        /// ShadowFar より小さい値にすること。
        /// </summary>
        float ShadowDistance = 30.0f;

        /// <summary>
        /// セルフシャドウ除去バイアス。
        /// アクネ(縞模様)が出たら増やす。影が浮く(Peter Pan)なら減らす。
        /// 推奨: 0.001 〜 0.01
        /// </summary>
        float ShadowBias = 0.005f;
    };

    struct PointLightComponent
    {
        DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };
        float             Intensity = 1.0f;
        float             Range = 10.0f;
        bool              IsActive = true;
    };

    struct SpotLightComponent
    {
        DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
        DirectX::XMFLOAT3 Color = { 1.0f,  1.0f, 1.0f };
        float             Intensity = 1.0f;
        float             Range = 10.0f;
        float             InnerConeRad = 0.2f;
        float             OuterConeRad = 0.4f;
        bool              IsActive = true;
    };

} // namespace ecs
