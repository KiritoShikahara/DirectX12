#pragma once

#include <vector>
#include <string>
#include<filesystem>
#include <memory>
#include <DirectXMath.h>
#include <graphics/VertexBuffer/VertexBuffer.h>
#include <graphics/IndexBuffer/IndexBuffer.h>
#include<Utility/Export/Export.h>
#include"FbxData.h"

namespace graphics
{
    /// <summary>
	/// FBX モデルのリソースクラス
    /// </summary>
    class ENGINE_API FbxResource
    {
    public:
        FbxResource() = default;
        ~FbxResource() = default;

        // GPU オブジェクトを保持するため コピー・ムーブ 禁止
        FbxResource(const FbxResource&) = delete;
        FbxResource& operator=(const FbxResource&) = delete;
        FbxResource(FbxResource&&) = delete;
        FbxResource& operator=(FbxResource&&) = delete;

        /// <summary>
        /// .binをロードしてGPUバッファを作成
        /// </summary>
        /// <returns></returns>
        bool Load(ID3D12GraphicsCommandList* cmdList,
            const std::string& binPath,
            const std::string& anmPath = "");

        // GPU転送後にアップロードバッファを解放する
        void ReleaseUploadBuffers();

        bool IsLoaded()      const { return mIsLoaded; }
        bool HasSkinning()   const { return !mBones.empty(); }
        bool HasAnimation()  const { return !mAnimClips.empty(); }
        int  GetBoneCount()  const { return static_cast<int>(mBones.size()); }
        int  GetClipCount()  const { return static_cast<int>(mAnimClips.size()); }


        /// <summary>セクション（マテリアル単位）の一覧。テクスチャ設定に使用。</summary>
        std::vector<FbxSection>& GetSections() { return mSections; }
        const std::vector<FbxSection>& GetSections() const { return mSections; }

        const std::vector<FbxBoneData>& GetBones()     const { return mBones; }
        const std::vector<FbxAnimClip>& GetAnimClips() const { return mAnimClips; }

        /*
        * GPU バッファ
        */
        VertexBuffer* GetVertexBuffer() const { return mVB.get(); }
        IndexBuffer* GetIndexBuffer()  const { return mIB.get(); }
    private:
        // 内部ロード処理
        bool LoadBin(const std::string& path,
            std::vector<FbxVertex>& outVerts,
            std::vector<uint32_t>& outIndices);
        bool LoadAnm(const std::string& path);
        bool UploadGPU(ID3D12GraphicsCommandList* cmdList,
            const std::vector<FbxVertex>& verts,
            const std::vector<uint32_t>& indices);

        void ResolveTextures(const std::filesystem::path& textureDir);

        static std::string ReadString(FILE* fp);

        bool mIsLoaded = false;

        std::unique_ptr<VertexBuffer> mVB;
        std::unique_ptr<IndexBuffer>  mIB;

        std::vector<FbxSection>   mSections;
        std::vector<FbxBoneData>  mBones;
        std::vector<FbxAnimClip>  mAnimClips;
    };
}


