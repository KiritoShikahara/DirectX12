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
        // アトラスUV座標
        float uvX0 = 0.f, uvY0 = 0.f;
        float uvX1 = 0.f, uvY1 = 0.f;

        // em空間でのレイアウト情報
        float planeBearingX = 0.f;  // left
        float planeBearingY = 0.f;  // top
        float planeWidth = 0.f;
        float planeHeight = 0.f;

        // 次の文字への水平移動量
        float advance = 0.f;
    };

    /// <summary>
    /// MSDFフォントアトラス
    /// </summary>
    class ENGINE_API TextAtlas
    {
    public:
        TextAtlas() = default;
        ~TextAtlas() = default;

        TextAtlas(const TextAtlas&) = delete;
        TextAtlas& operator=(const TextAtlas&) = delete;

        /// <summary>
        /// アトラスの読み込み
        /// </summary>
        bool Load(const std::filesystem::path& pngPath,
            const std::string& jsonPath);

        void Release();

        // グリフ情報
        const GlyphInfo* GetGlyph(uint32_t codepoint) const;

        // メトリクス
        float GetEmSize()     const { return mEmSize; }
        float GetLineHeight() const { return mLineHeight; }
        float GetAscender()   const { return mAscender; }
        float GetDescender()  const { return mDescender; }
        float GetPxRange()    const { return mPxRange; }

        // GPUハンドル
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