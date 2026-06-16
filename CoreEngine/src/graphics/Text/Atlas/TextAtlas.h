#pragma once

#include <graphics/Texture/Texture.h>
#include <Utility/Export/Export.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace graphics
{
    /// <summary>
    /// 1文字分のアトラス情報
    /// </summary>
    struct GlyphInfo
    {
        /// <summary>アトラス UV 座標</summary>
        float uvX0 = 0.f, uvY0 = 0.f;
        float uvX1 = 0.f, uvY1 = 0.f;

        /// <summary>em 空間でのレイアウト情報</summary>
        float planeBearingX = 0.f;  // left
        float planeBearingY = 0.f;  // top
        float planeWidth = 0.f;
        float planeHeight = 0.f;

        /// <summary>次の文字への水平移動量 (em 単位)</summary>
        float advance = 0.f;
    };

    /// <summary>
    /// MSDF フォントアトラス。
    /// font.png + font.json を読み込み GlyphMap を構築する。
    /// テクスチャ管理は既存の Texture クラスに委譲する。
    /// </summary>
    class ENGINE_API TextAtlas
    {
    public:
        TextAtlas() = default;
        ~TextAtlas() = default;

        TextAtlas(const TextAtlas&) = delete;
        TextAtlas& operator=(const TextAtlas&) = delete;

        /// <summary>
        /// アトラスを読み込む。
        /// Texture::Create() が DX12Device / GDescriptorHeapManager を
        /// シングルトンから取得するため、引数への依存なし。
        /// </summary>
        bool Load(const std::filesystem::path& pngPath,
            const std::string& jsonPath);

        void Release();

        // -----------------------------------------------------------------------
        //  グリフ情報
        // -----------------------------------------------------------------------
        const GlyphInfo* GetGlyph(uint32_t codepoint) const;

        // -----------------------------------------------------------------------
        //  メトリクス
        // -----------------------------------------------------------------------
        float GetEmSize()     const { return mEmSize; }
        float GetLineHeight() const { return mLineHeight; }
        float GetAscender()   const { return mAscender; }
        float GetDescender()  const { return mDescender; }
        float GetPxRange()    const { return mPxRange; }

        // -----------------------------------------------------------------------
        //  GPU ハンドル
        // -----------------------------------------------------------------------
        D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const;
        bool IsLoaded() const { return mIsLoaded; }

    private:
        bool ParseJson(const std::string& jsonPath);

        /// <summary>PNG テクスチャ。既存 Texture クラスに委譲。</summary>
        Texture*  mTexture = nullptr;

        std::unordered_map<uint32_t, GlyphInfo> mGlyphs;

        float    mEmSize = 48.f;
        float    mLineHeight = 1.2f;
        float    mAscender = 0.8f;
        float    mDescender = -0.2f;
        float    mPxRange = 4.f;
        uint32_t mAtlasW = 512;
        uint32_t mAtlasH = 512;

        bool mIsLoaded = false;
    };

} // namespace graphics