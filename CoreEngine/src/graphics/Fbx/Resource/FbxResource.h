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

    /// <summary>
    /// .bin / .anm ファイルを読み込んで GPU バッファを構築するクラス
    /// </summary>
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
        /// </summary>
        bool Load(const std::string& binPath);

        /// <summary>
        /// マテリアル情報
        /// </summary>
        struct MaterialInfo
        {
            std::string Name;
            std::string AlbedoPath, NormalPath, MetallicPath, RoughnessPath, AOPath, EmissivePath;
            DirectX::XMFLOAT3 BaseColorFactor = {};
            float MetallicFactor = 0.f;
            float RoughnessFactor = 0.f;
            DirectX::XMFLOAT3 EmissiveFactor = {};
            uint32_t IndexCount = 0;
            uint32_t IndexOffset = 0;
        };

        /// <summary>
        /// ロードされたバイナリデータ
        /// </summary>
        struct LoadedBinData
        {
            bool Success = false;
            std::vector<FbxVertex> Vertices;
            std::vector<uint32_t> Indices;
            std::vector<MaterialInfo> Materials;
            std::vector<FbxBoneData> Bones;
            DirectX::XMFLOAT3 BottomCenterPivot = {};
            std::filesystem::path TextureBaseDir;
        };

        /// <summary>
        /// .bin ファイルのパース
        /// </summary>
        static LoadedBinData LoadBinData(const std::string& binPath);

        /// <summary>
        /// テクスチャパスの収集
        /// </summary>
        static void CollectTexturePaths(
            const LoadedBinData& data,
            std::vector<std::filesystem::path>& outSrgbPaths,
            std::vector<std::filesystem::path>& outLinearPaths);

        /// <summary>
        /// バイナリデータから GPU リソースを構築する
        /// </summary>
        bool CreateFromBinData(const LoadedBinData& data);

        /// <summary>
        /// .anm ファイルを追加でロードする
        /// </summary>
        bool LoadAnm(
            const std::string& anmPath,
            const std::string& clipName = "");

        /// <summary>
        /// ロード済みか
        /// </summary>
        bool IsLoaded()    const { return mIsLoaded; }

        /// <summary>
        /// スキニングを持つか
        /// </summary>
        bool HasSkinning() const { return !mBones.empty(); }

        /// <summary>
        /// アニメーションを持つか
        /// </summary>
        bool HasAnimation()const { return !mAnimClips.empty(); }

        /// <summary>
        /// ボーン数を取得
        /// </summary>
        int  GetBoneCount()const { return static_cast<int>(mBones.size()); }

        /// <summary>
        /// セクションリストを取得
        /// </summary>
        const std::vector<FbxSection>& GetSections()  const { return mSections; }

        /// <summary>
        /// セクションリストを取得
        /// </summary>
        std::vector<FbxSection>& GetSections() { return mSections; }

        /// <summary>
        /// ボーンデータを取得
        /// </summary>
        const std::vector<FbxBoneData>& GetBones()     const { return mBones; }

        /// <summary>
        /// アニメーションクリップを取得
        /// </summary>
        const std::vector<FbxAnimClip>& GetAnimClips() const { return mAnimClips; }

        /// <summary>
        /// 底面中心ピボットを取得
        /// </summary>
        const DirectX::XMFLOAT3& GetBottomCenterPivot() const { return mBottomCenterPivot; }

        /// <summary>
        /// クリップ名からインデックスを返す
        /// </summary>
        int FindClipIndex(const std::string& name) const;

        /// <summary>
        /// メモリ上の情報から直接 GPU バッファを構築する
        /// </summary>
        bool BuildFromMemory(
            const std::vector<FbxVertex>& vertices,
            const std::vector<uint32_t>& indices,
            const std::vector<FbxSection>& sections);

        /// <summary>
        /// VB と IB をコマンドリストにセットする
        /// </summary>
        void SetBuffers(ID3D12GraphicsCommandList* cmdList) const;

    private:
        /// <summary>
        /// バイナリファイルの内部ロード
        /// </summary>
        bool LoadBin(const std::string& binPath);

        /// <summary>
        /// 描画セクションリスト
        /// </summary>
        std::vector<FbxSection>   mSections;

        /// <summary>
        /// ボーンデータリスト
        /// </summary>
        std::vector<FbxBoneData>  mBones;

        /// <summary>
        /// アニメーションクリップリスト
        /// </summary>
        std::vector<FbxAnimClip>  mAnimClips;

        /// <summary>
        /// 頂点バッファ
        /// </summary>
        std::unique_ptr<VertexBuffer> mVB;

        /// <summary>
        /// インデックスバッファ
        /// </summary>
        std::unique_ptr<IndexBuffer>  mIB;

        /// <summary>
        /// 底面中心オフセット
        /// </summary>
        DirectX::XMFLOAT3 mBottomCenterPivot = { 0.f, 0.f, 0.f };

        /// <summary>
        /// ロード済みフラグ
        /// </summary>
        bool mIsLoaded = false;
    };

} // namespace graphics