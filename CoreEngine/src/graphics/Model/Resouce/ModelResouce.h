#pragma once

#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <Utility/Export/Export.h>

#include <graphics/VertexBuffer/VertexBuffer.h>
#include <graphics/IndexBuffer/IndexBuffer.h>
#include<graphics/Model/ModelData.h>


namespace graphics
{
    class ENGINE_API ModelResource
    {
    public:
        ModelResource() = default;
        ~ModelResource() = default;

        ModelResource(const ModelResource&) = delete;
        ModelResource& operator=(const ModelResource&) = delete;
        ModelResource(ModelResource&&) = delete;
        ModelResource& operator=(ModelResource&&) = delete;

        /// <summary>
        /// .bin を読み込んで GPU バッファを構築する。
        /// アニメーションは後から AppendAnimation() で追加する。
        /// </summary>
        bool Load(const std::string& binPath);

        /// <summary>
        /// .anm を追加読み込みしてクリップを追記する。
        /// 同じリソースに何度でも呼べる（クリップが末尾に追加される）。
        /// overrideName を指定するとクリップ名を上書きできる。
        ///   クリップ1つ  → overrideName をそのまま使用
        ///   クリップ複数 → "overrideName_0", "overrideName_1" ... と連番付与
        /// </summary>
        bool AppendAnimation(const std::string& anmPath,
            const std::string& overrideName = "");

        // ── 状態 ──────────────────────────────────────────────
        bool IsLoaded()     const { return mIsLoaded; }
        bool HasSkinning()  const { return !mBones.empty(); }
        bool HasAnimation() const { return !mAnimClips.empty(); }
        int  GetBoneCount() const { return static_cast<int>(mBones.size()); }
        int  GetClipCount() const { return static_cast<int>(mAnimClips.size()); }

        /// <summary>クリップ名からインデックスを検索 (-1 = 見つからず)</summary>
        int FindClipIndex(const std::string& clipName) const;

        // ── データアクセサ ─────────────────────────────────────
        const std::vector<ModelSection>& GetSections()  const { return mSections; }
        std::vector<ModelSection>& GetSections() { return mSections; }
        const std::vector<ModelBoneData>& GetBones()     const { return mBones; }
        const std::vector<ModelAnimClip>& GetAnimClips() const { return mAnimClips; }

        VertexBuffer* GetVertexBuffer() const { return mVB.get(); }
        IndexBuffer* GetIndexBuffer()  const { return mIB.get(); }

    private:
        bool LoadBin(const std::string& path,
            std::vector<ModelVertex>& outVerts,
            std::vector<uint32_t>& outIndices);
        bool LoadAnm(const std::string& path);
        bool UploadGPU(const std::vector<ModelVertex>& verts,
            const std::vector<uint32_t>& indices);
        void ResolveTextures(const std::filesystem::path& baseDir);
        void ResolveBoneIndices();

        bool mIsLoaded = false;

        std::unique_ptr<VertexBuffer> mVB;
        std::unique_ptr<IndexBuffer>  mIB;

        std::vector<ModelSection>  mSections;
        std::vector<ModelBoneData> mBones;
        std::vector<ModelAnimClip> mAnimClips;
    };

} // namespace graphics