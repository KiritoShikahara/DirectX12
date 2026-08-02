#pragma once

#include <Utility/Export/Export.h>
#include <graphics/Color/Color.h>
#include <DirectXMath.h>

namespace graphics
{
    class Texture;
}

namespace ecs
{
    /// <summary>
    /// スプライトの描画レイヤー
    /// </summary>
    enum class ENGINE_API SpriteLayer : int
    {
        Background = 0,
        Map = 1000,
        Character = 2000,
        Effect = 3000,
        UI = 4000,
        System = 5000,
    };

    /// <summary>
    /// 塗りつぶしのタイプ
    /// </summary>
    enum class ENGINE_API FillType : int
    {
        Horizontal = 0,
        Radial = 1,
    };

    /// <summary>
    /// スプライト描画に必要なパラメータをまとめたコンポーネント
    /// </summary>
    struct ENGINE_API Sprite
    {
        /// <summary>乗算カラー</summary>
        graphics::Color Color = graphics::Color::White;

        /// <summary>基準点（ピボット）</summary>
        DirectX::XMFLOAT2 Pivot = { 0.0f, 0.0f };

        /// <summary>描画サイズ（ピクセル）</summary>
        DirectX::XMFLOAT2 Size = { 0.0f, 0.0f };

        /// <summary>サイズに対する追加倍率</summary>
        DirectX::XMFLOAT2 DrawScale = { 1.0f, 1.0f };

        /// <summary>反転フラグ</summary>
        DirectX::XMFLOAT2 Flip = { 1.0f, 1.0f };

        /// <summary>テクスチャUV切り出し範囲のスケール</summary>
        DirectX::XMFLOAT2 UVScale = { 1.0f, 1.0f };

        /// <summary>テクスチャUV切り出し範囲のオフセット</summary>
        DirectX::XMFLOAT2 UVOffset = { 0.0f, 0.0f };

        /// <summary>光度（輝度倍率）</summary>
        float Intensity = 1.0f;

        /// <summary>表示割合</summary>
        float FillAmount = 1.0f;

        /// <summary>塗りつぶしタイプ</summary>
        FillType FType = FillType::Horizontal;

        /// <summary>描画順</summary>
        int Layer = static_cast<int>(SpriteLayer::Character);

        /// <summary>テクスチャリソース（非所有）</summary>
        graphics::Texture* Texture = nullptr;

        /// <summary>表示フラグ</summary>
        bool IsVisible = true;

        /// <summary>コンストラクタ</summary>
        explicit Sprite(graphics::Texture* texture);

        /// <summary>レイヤー設定用のヘルパー</summary>
        void SetLayer(SpriteLayer base, int offset = 0);

        /// <summary>スプライトシートの行列数を指定してUVScaleを一括設定するヘルパー</summary>
        void SetSheetGrid(int columns, int rows);

        /// <summary>フレーム番号からUVOffsetを計算して設定するヘルパー</summary>
        void SetFrame(int frameIndex, int columns);
    };
}