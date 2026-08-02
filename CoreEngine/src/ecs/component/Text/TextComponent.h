#pragma once

#include <DirectXMath.h>
#include <string>

namespace ecs
{
    /// <summary>
    /// テキスト描画コンポーネント。
    /// Transform は使わず、スクリーン座標を直接持つ。
    /// </summary>
    struct TextComponent
    {
        /// <summary>描画する文字列</summary>
        std::wstring Text = L"";

        /// <summary>左上基準のスクリーン座標 (px)</summary>
        float             X = 0.f;
        float             Y = 0.f;

        /// <summary>文字高さ (px)</summary>
        float             Size = 32.f;

        /// <summary>文字色 RGBA (0–1)</summary>
        DirectX::XMFLOAT4 Color = { 1.f, 1.f, 1.f, 1.f };

        /// <summary>描画順（小さいほど奥）</summary>
        int               Layer = 0;

        /// <summary>false のとき描画スキップ</summary>
        bool              IsVisible = true;
    };

} // namespace ecs