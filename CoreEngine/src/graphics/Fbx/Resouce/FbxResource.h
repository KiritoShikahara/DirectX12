#pragma once

#include <vector>
#include <string>
#include <memory>
#include <DirectXMath.h>
#include <graphics/VertexBuffer/VertexBuffer.h>
#include <graphics/IndexBuffer/IndexBuffer.h>
#include<Utility/Export/Export.h>

namespace graphics
{
	class Texture;

    /// <summary>
    /// GPU 頂点レイアウト
    /// </summary>
    struct FbxVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 UV;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 Tangent;
        DirectX::XMFLOAT4 Color;
        int32_t           Bone[4];
        float             Weight[4];
    };

    /// <summary>
	/// マテリアルごとのセクション情報
    /// </summary>
    struct ENGINE_API FbxSection
    {
        std::string MaterialName;

        // テクスチャファイルパス（BreakTexturePath() 済みのファイル名のみ）
        std::string DiffuseTexturePath;
        std::string NormalTexturePath;
        std::string MetallicTexturePath;
        std::string RoughnessTexturePath;

        // テクスチャ未設定時のパラメータ
        DirectX::XMFLOAT4 BaseColor = { 1.f, 1.f, 1.f, 1.f };
        float             Metallic = 0.f;
        float             Roughness = 0.5f;

        // インデックスバッファ内の範囲（Load 後に自動計算）
        uint32_t IndexOffset = 0;   ///< バッファ先頭からの要素オフセット
        uint32_t IndexCount = 0;   ///< このセクションのインデックス数

        // TextureManager 等から外部で解決して設定する
        Texture* DiffuseTexture = nullptr;
        Texture* NormalTexture = nullptr;
    };

    /// <summary>
    /// ボーン情報
    /// </summary>
    struct ENGINE_API FbxBoneData
    {
        std::string           Name;
        int32_t               ParentIndex = -1;
        DirectX::XMFLOAT4X4   BindMatrix;
    };

    /// <summary>
	/// アニメーションクリップ情報
    /// </summary>
    struct ENGINE_API FbxAnimClip
    {
        std::string Name;
        int32_t     NumFrame = 0;
        float       StartTime = 0.f;
        float       StopTime = 0.f;
        std::vector<std::vector<DirectX::XMFLOAT4X4>> KeyFrames;
    };

    /// <summary>
    /// GPU 転送用インスタンスデータ
    /// StructuredBuffer<FbxInstanceData> として t0 にバインドする
    /// </summary>
    struct alignas(16) FbxInstanceData
    {
        DirectX::XMFLOAT4X4 World;       ///< ワールド行列（転置済み）
        DirectX::XMFLOAT4   BaseColor;   ///< マテリアルベースカラー
        float               Metallic;    ///< メタリック値
        float               Roughness;   ///< ラフネス値
        float               Intensity;   ///< 輝度倍率
        uint32_t            BoneOffset;  ///< ボーン行列バッファ内の先頭インデックス
        uint32_t            BoneCount;   ///< 使用するボーン数（0 でスキニングなし）
        float               _pad[3];     ///< 16byte アライン用パディング
    };

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


