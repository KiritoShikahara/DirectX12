#pragma once
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <Utility/Export/Export.h>

#include <graphics/VertexBuffer/VertexBuffer.h>
#include <graphics/IndexBuffer/IndexBuffer.h>
#include <graphics/FBX/Data/FbxData.h>

struct ID3D12GraphicsCommandList;

namespace graphics
{
    class TextureManager;

    // ============================================================
    //  FbxResource
    //  .bin / .anm ファイルを読み込んで GPU バッファを構築する
    //  ModelResource 相当
    // ============================================================
    class ENGINE_API FbxResource
    {
    public:
        FbxResource() = default;
        ~FbxResource() = default;

        FbxResource(const FbxResource&) = delete;
        FbxResource& operator=(const FbxResource&) = delete;
        FbxResource(FbxResource&&) = delete;
        FbxResource& operator=(FbxResource&&) = delete;

        /// <summary>
        /// .bin ファイルを読み込んで GPU バッファを構築する
        /// CreateStaticSync を使うので cmdList 不要
        /// </summary>
        bool Load(const std::string& binPath);

        /// <summary>
        /// .anm ファイルを追加でロードする
        /// 複数回呼び出せるので1モデルに複数アニメーションを持てる
        /// clipName を省略するとファイル名(拡張子なし)をクリップ名にする
        /// </summary>
        bool LoadAnm(
            const std::string& anmPath,
            const std::string& clipName = "");

        // ── 状態 ──────────────────────────────────────────────
        bool IsLoaded()    const { return mIsLoaded; }
        bool HasSkinning() const { return !mBones.empty(); }
        bool HasAnimation()const { return !mAnimClips.empty(); }
        int  GetBoneCount()const { return static_cast<int>(mBones.size()); }

        // ── データアクセサ ─────────────────────────────────────
        const std::vector<FbxSection>& GetSections()  const { return mSections; }
        std::vector<FbxSection>& GetSections() { return mSections; }
        const std::vector<FbxBoneData>& GetBones()     const { return mBones; }
        const std::vector<FbxAnimClip>& GetAnimClips() const { return mAnimClips; }

        /// <summary>クリップ名からインデックスを返す (-1: 見つからない)</summary>
        int FindClipIndex(const std::string& name) const;

        /// <summary>VB/IB をコマンドリストにセットする</summary>
        void SetBuffers(ID3D12GraphicsCommandList* cmdList) const;

    private:
        bool LoadBin(const std::string& binPath);

        std::vector<FbxSection>   mSections;
        std::vector<FbxBoneData>  mBones;
        std::vector<FbxAnimClip>  mAnimClips;

        std::unique_ptr<VertexBuffer> mVB;
        std::unique_ptr<IndexBuffer>  mIB;

        bool mIsLoaded = false;
    };

} // namespace graphics