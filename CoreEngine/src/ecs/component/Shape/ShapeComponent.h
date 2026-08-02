#pragma once

#include "../sprite/SpriteComponent.h"

namespace ecs
{
    /// <summary>
    /// 図形描画の種類
    /// </summary>
    enum class ENGINE_API ShapeType : int
    {
        Rect = 0,
        Circle = 1,
        Triangle = 2,
    };

    /// <summary>
    /// テクスチャを使わない単色図形描画コンポーネント
    /// </summary>
    struct ENGINE_API Shape
    {
        /// <summary>描画色</summary>
        graphics::Color Color = graphics::Color::White;

        /// <summary>基準点（ピボット）</summary>
        DirectX::XMFLOAT2 Pivot = { 0.5f, 0.5f };

        /// <summary>描画サイズ（ピクセル）</summary>
        DirectX::XMFLOAT2 Size = { 100.0f, 100.0f };

        /// <summary>サイズに対する追加倍率</summary>
        DirectX::XMFLOAT2 DrawScale = { 1.0f, 1.0f };

        /// <summary>反転フラグ</summary>
        DirectX::XMFLOAT2 Flip = { 1.0f, 1.0f };

        /// <summary>輝度</summary>
        float Intensity = 1.0f;

        /// <summary>表示割合</summary>
        float FillAmount = 1.0f;

        /// <summary>塗りつぶしタイプ</summary>
        FillType FType = FillType::Horizontal;

        /// <summary>描画順</summary>
        int Layer = static_cast<int>(SpriteLayer::Character);

        /// <summary>図形の種類</summary>
        ShapeType Type = ShapeType::Rect;

        /// <summary>表示フラグ</summary>
        bool IsVisible = true;

        Shape() = default;
        explicit Shape(ShapeType type) : Type(type) {}

        /// <summary>レイヤー設定用のヘルパー</summary>
        void SetLayer(SpriteLayer base, int offset = 0)
        {
            Layer = static_cast<int>(base) + offset;
        }
    };
}