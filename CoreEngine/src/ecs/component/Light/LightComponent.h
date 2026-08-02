#pragma once

#include <DirectXMath.h>
#include <cstdint>

namespace ecs
{
    /// <summary>
    /// ライトの種類
    /// </summary>
    enum class eLightType : uint32_t
    {
        Directional = 0,
        Point = 1,
        Spot = 2,
    };

    /// <summary>
    /// ディレクショナルライトコンポーネント
    /// </summary>
    struct DirectionalLightComponent
    {
        /// <summary>ライトの方向</summary>
        DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
        /// <summary>ライトの色</summary>
        DirectX::XMFLOAT3 Color = { 1.0f,  1.0f, 1.0f };
        /// <summary>光度（輝度倍率）</summary>
        float             Intensity = 1.0f;
        /// <summary>有効フラグ</summary>
        bool              IsActive = true;

        /// <summary>true にすると Shadow Map を生成してシャドウを落とす</summary>
        bool  CastShadow = false;

        /// <summary>正射影の範囲 (幅・高さ共通)</summary>
        float ShadowRange = 50.0f;

        /// <summary>ライトカメラの near クリップ</summary>
        float ShadowNear = 0.1f;
        /// <summary>ライトカメラの far クリップ</summary>
        float ShadowFar = 200.0f;

        /// <summary>ライトカメラの注視点 (ワールド空間)</summary>
        DirectX::XMFLOAT3 ShadowTarget = { 0.0f, 0.0f, 0.0f };

        /// <summary>ライト位置をターゲットからどれだけ離すか</summary>
        float ShadowDistance = 30.0f;

        /// <summary>セルフシャドウ除去バイアス</summary>
        float ShadowBias = 0.005f;
    };

    /// <summary>
    /// ポイントライトコンポーネント
    /// </summary>
    struct PointLightComponent
    {
        /// <summary>ライトの色</summary>
        DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };
        /// <summary>光度（輝度倍率）</summary>
        float             Intensity = 1.0f;
        /// <summary>影響範囲の半径</summary>
        float             Range = 10.0f;
        /// <summary>有効フラグ</summary>
        bool              IsActive = true;
    };

    /// <summary>
    /// スポットライトコンポーネント
    /// </summary>
    struct SpotLightComponent
    {
        /// <summary>ライトの方向</summary>
        DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
        /// <summary>ライトの色</summary>
        DirectX::XMFLOAT3 Color = { 1.0f,  1.0f, 1.0f };
        /// <summary>光度（輝度倍率）</summary>
        float             Intensity = 1.0f;
        /// <summary>影響範囲の半径</summary>
        float             Range = 10.0f;
        /// <summary>内側の照射角（ラジアン）</summary>
        float             InnerConeRad = 0.2f;
        /// <summary>外側の照射角（ラジアン）</summary>
        float             OuterConeRad = 0.4f;
        /// <summary>有効フラグ</summary>
        bool              IsActive = true;
    };
}