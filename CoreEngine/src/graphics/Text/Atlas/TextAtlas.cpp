#include "pch.h"
#include "TextAtlas.h"

#include <json/json.hpp>
#include <fstream>
#include<graphics/Texture/TextureManager.h>

namespace graphics
{
    bool TextAtlas::Load(const std::filesystem::path& pngPath,
        const std::string& jsonPath)
    {
        Release();

        if (!ParseJson(jsonPath)) return false;

        // PNG は既存 Texture::Create() (DirectXTex WIC ルート) に委譲
        mTexture = TextureManager::Get().GetOrLoad(pngPath);
        if (mTexture == nullptr)
        {
            return false;
        }

        // Texture から実サイズを反映
        mAtlasW = static_cast<uint32_t>(mTexture->GetWidth());
        mAtlasH = static_cast<uint32_t>(mTexture->GetHeight());

        mIsLoaded = true;
        DEBUG_LOG(sys::eLogLevel::Log,
            "TextAtlas: Loaded. glyphs={}, atlas={}x{}, emSize={}, pxRange={}",
            mGlyphs.size(), mAtlasW, mAtlasH, mEmSize, mPxRange);

        return true;
    }

    void TextAtlas::Release()
    {
        mTexture = nullptr;
        mGlyphs.clear();
        mIsLoaded = false;
    }

    const GlyphInfo* TextAtlas::GetGlyph(uint32_t codepoint) const
    {
        auto it = mGlyphs.find(codepoint);
        return (it != mGlyphs.end()) ? &it->second : nullptr;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE TextAtlas::GetSrvGpuHandle() const
    {
        return mTexture->GetGpuHandle();
    }

    // -----------------------------------------------------------------------
    //  JSON パース  (msdf-atlas-gen 出力フォーマット)
    // -----------------------------------------------------------------------
    bool TextAtlas::ParseJson(const std::string& jsonPath)
    {
        std::ifstream f(jsonPath);
        if (!f.is_open())
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "TextAtlas: Cannot open json: {}", jsonPath);
            return false;
        }

        nlohmann::json root;
        try { f >> root; }
        catch (const std::exception& e)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "TextAtlas: JSON parse error: {}", e.what());
            return false;
        }

        // atlas メタ情報
        if (root.contains("atlas"))
        {
            auto& a = root["atlas"];
            mAtlasW = a.value("width", 512u);
            mAtlasH = a.value("height", 512u);
            mEmSize = a.value("size", 48.f);
            mPxRange = a.value("pxRange", 4.f);
        }

        if (root.contains("metrics"))
        {
            auto& m = root["metrics"];
            mLineHeight = m.value("lineHeight", 1.2f);
            mAscender = m.value("ascender", 0.8f);
            mDescender = m.value("descender", -0.2f);
        }

        if (!root.contains("glyphs")) return true;

        const float invW = 1.f / static_cast<float>(mAtlasW);
        const float invH = 1.f / static_cast<float>(mAtlasH);

        for (auto& g : root["glyphs"])
        {
            GlyphInfo info{};
            const uint32_t cp = g.value("unicode", 0u);
            info.advance = g.value("advance", 0.f);

            if (g.contains("planeBounds"))
            {
                auto& pb = g["planeBounds"];
                info.planeBearingX = pb.value("left", 0.f);
                info.planeBearingY = pb.value("top", 0.f);
                info.planeWidth = pb.value("right", 0.f) - info.planeBearingX;
                info.planeHeight = info.planeBearingY - pb.value("bottom", 0.f);
            }

            if (g.contains("atlasBounds"))
            {
                auto& ab = g["atlasBounds"];
                info.uvX0 = ab.value("left", 0.f) * invW;
                info.uvX1 = ab.value("right", 0.f) * invW;
                info.uvY0 = (mAtlasH - ab.value("top", 0.f)) * invH;
                info.uvY1 = (mAtlasH - ab.value("bottom", 0.f)) * invH;
            }

            mGlyphs[cp] = info;
        }

        return true;
    }

} // namespace graphics