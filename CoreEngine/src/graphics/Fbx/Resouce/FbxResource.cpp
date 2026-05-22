#include"pch.h"
#include "FbxResource.h"

#include<graphics/Texture/TextureManager.h>
#include<graphics/Texture/Texture.h>

namespace graphics
{
    /// <summary>
    /// 内部ヘルパー
    /// </summary>
    /// <returns></returns>
    std::string FbxResource::ReadString(FILE* fp)
    {
        int32_t length = 0;
        if (fread(&length, sizeof(int32_t), 1, fp) != 1 || length <= 0)
        {
            return {};
        }
        std::string str(length, '\0');
        fread(str.data(), sizeof(char), length, fp);
        return str;
    }

    /// <summary>
    /// .bin + .anm を読み込み GPU バッファを構築する。
    /// 二重ロード呼び出しを防ぐため IsLoaded() チェックは呼び出し元で行うこと。
    /// </summary>
    bool FbxResource::Load(ID3D12GraphicsCommandList* cmdList,
        const std::string& binPath,
        const std::string& anmPath)
    {
        std::vector<FbxVertex>  verts;
        std::vector<uint32_t>   indices;

        if (!LoadBin(binPath, verts, indices))
        {
            return false;
        }

        if (!anmPath.empty())
        {
            if (!LoadAnm(anmPath))
            {
                // アニメーション読み込み失敗はワーニング止まり（静止メッシュとして続行）
                DEBUG_LOG(sys::eLogLevel::Warning,
                    std::format("FbxResource: Failed to load animation: {}", anmPath));
            }
        }

        if (!UploadGPU(cmdList,verts, indices))
        {
            return false;
        }

        ResolveTextures(std::filesystem::path(binPath).parent_path());

        mIsLoaded = true;
        DEBUG_LOG(sys::eLogLevel::Log,
            std::format("FbxResource: Loaded '{}' ({} sections, {} bones, {} clips)",
                binPath, mSections.size(), mBones.size(), mAnimClips.size()));
        return true;
    }

    /// <summary>
    /// GPU実行後にUploadヒープを解放
    /// </summary>
    void FbxResource::ReleaseUploadBuffers()
    {
        if (mVB) mVB->ReleaseUploadBuffer();
        if (mIB) mIB->ReleaseUploadBuffer();
    }

    /// <summary>
    /// FbxAnalyzer::Convert() が書き出したバイナリ形式に従って読み込む。
    /// </summary>
    /// <returns></returns>
    bool FbxResource::LoadBin(const std::string& path,
        std::vector<FbxVertex>& outVerts,
        std::vector<uint32_t>& outIndices)
    {
        FILE* fp = nullptr;
        if (fopen_s(&fp, path.c_str(), "rb") != 0 || !fp)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                std::format("FbxResource: File not found: {}", path));
            return false;
        }

        // ---- ヘッダ ----
        int32_t meshCount = 0;
        int32_t polygonCount = 0;
        int32_t vertexCount = 0;
        fread(&meshCount, sizeof(int32_t), 1, fp);
        fread(&polygonCount, sizeof(int32_t), 1, fp);
        fread(&vertexCount, sizeof(int32_t), 1, fp);

        // ---- 頂点 ----
        outVerts.resize(vertexCount);
        for (int i = 0; i < vertexCount; ++i)
        {
            FbxVertex& v = outVerts[i];
            fread(&v.Position, sizeof(DirectX::XMFLOAT3), 1, fp);
            fread(&v.UV, sizeof(DirectX::XMFLOAT2), 1, fp);
            fread(&v.Normal, sizeof(DirectX::XMFLOAT3), 1, fp);
            fread(&v.Tangent, sizeof(DirectX::XMFLOAT3), 1, fp);
            fread(&v.Color, sizeof(DirectX::XMFLOAT4), 1, fp);
            fread(v.Bone, sizeof(int32_t) * 4, 1, fp);
            fread(v.Weight, sizeof(float) * 4, 1, fp);
        }

        // ---- インデックス ----
        int32_t indexCount = 0;
        fread(&indexCount, sizeof(int32_t), 1, fp);
        outIndices.resize(indexCount);
        fread(outIndices.data(), sizeof(uint32_t), indexCount, fp);

        // ---- マテリアル → FbxSection ----
        int32_t matCount = 0;
        fread(&matCount, sizeof(int32_t), 1, fp);
        mSections.resize(matCount);

        // IndexOffset は各マテリアルのポリゴン数（PolygonCount）を
        // 前方から積算することで復元する（FbxAnalyzer の書き込み順に依存）
        uint32_t indexCursor = 0;
        for (int i = 0; i < matCount; ++i)
        {
            FbxSection& sec = mSections[i];
            sec.MaterialName = ReadString(fp);
            sec.DiffuseTexturePath = ReadString(fp);
            sec.NormalTexturePath = ReadString(fp);
            sec.MetallicTexturePath = ReadString(fp);
            sec.RoughnessTexturePath = ReadString(fp);
            fread(&sec.BaseColor, sizeof(DirectX::XMFLOAT4), 1, fp);
            fread(&sec.Metallic, sizeof(float), 1, fp);
            fread(&sec.Roughness, sizeof(float), 1, fp);

            uint32_t polyCount = 0;
            fread(&polyCount, sizeof(uint32_t), 1, fp);

            sec.IndexOffset = indexCursor;
            sec.IndexCount = polyCount * 3u;
            indexCursor += sec.IndexCount;
        }

        // ---- ボーン ----
        int32_t boneCount = 0;
        fread(&boneCount, sizeof(int32_t), 1, fp);
        mBones.resize(boneCount);
        for (int i = 0; i < boneCount; ++i)
        {
            mBones[i].Name = ReadString(fp);
            fread(&mBones[i].ParentIndex, sizeof(int32_t), 1, fp);
            fread(&mBones[i].BindMatrix, sizeof(DirectX::XMFLOAT4X4), 1, fp);
        }

        fclose(fp);
        return true;
    }

    /// <summary>
	/// .bin と同様に FbxAnalyzer::Convert() が書き出したバイナリ形式に従って読み込む。
    /// </summary>
    /// <returns></returns>
    bool FbxResource::LoadAnm(const std::string& path)
    {
        FILE* fp = nullptr;
        if (fopen_s(&fp, path.c_str(), "rb") != 0 || !fp)
        {
            return false;
        }

        int32_t clipCount = 0;
        fread(&clipCount, sizeof(int32_t), 1, fp);
        mAnimClips.resize(clipCount);

        for (int c = 0; c < clipCount; ++c)
        {
            FbxAnimClip& clip = mAnimClips[c];
            clip.Name = ReadString(fp);
            fread(&clip.NumFrame, sizeof(int32_t), 1, fp);
            fread(&clip.StartTime, sizeof(float), 1, fp);
            fread(&clip.StopTime, sizeof(float), 1, fp);

            int32_t numBoneAnim = 0;
            fread(&numBoneAnim, sizeof(int32_t), 1, fp);
            clip.KeyFrames.resize(numBoneAnim);

            for (int b = 0; b < numBoneAnim; ++b)
            {
                int32_t frameCount = 0;
                fread(&frameCount, sizeof(int32_t), 1, fp);

                clip.KeyFrames[b].resize(frameCount);
                fread(clip.KeyFrames[b].data(),
                    sizeof(DirectX::XMFLOAT4X4), frameCount, fp);
            }
        }

        fclose(fp);
        return true;
    }

    bool FbxResource::UploadGPU(ID3D12GraphicsCommandList* cmdList,
        const std::vector<FbxVertex>& verts,
        const std::vector<uint32_t>& indices)
    {
        // VertexBuffer::CreateStatic(CmdList, InitData, Size, Stride) に合わせる
        mVB = std::make_unique<VertexBuffer>();
        const uint32_t vbSize = static_cast<uint32_t>(verts.size() * sizeof(FbxVertex));
        if (!mVB->CreateStatic(cmdList, verts.data(), vbSize, sizeof(FbxVertex)))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxResource: Failed to create vertex buffer.");
            return false;
        }

        // IndexBuffer::CreateStatic(CmdList, InitData, Size, Format) に合わせる
        mIB = std::make_unique<IndexBuffer>();
        const uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
        if (!mIB->CreateStatic(cmdList, indices.data(), ibSize, DXGI_FORMAT_R32_UINT))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxResource: Failed to create index buffer.");
            return false;
        }

        return true;
    }


    void FbxResource::ResolveTextures(const std::filesystem::path& textureDir)
    {
        auto& texManager = graphics::TextureManager::Get();

        for (auto& sec : mSections)
        {
            // ディフューズ
            if (!sec.DiffuseTexturePath.empty())
            {
                sec.DiffuseTexture = texManager.GetOrLoad(textureDir / sec.DiffuseTexturePath);
                if (!sec.DiffuseTexture)
                {
                    DEBUG_LOG(sys::eLogLevel::Warning,
                        std::format("FbxResource: Diffuse texture not found: {}",
                            sec.DiffuseTexturePath));
                }
            }

            // 法線マップ（存在しない場合はnullptrのまま、シェーダ側でフォールバック）
            if (!sec.NormalTexturePath.empty())
            {
                sec.NormalTexture = texManager.GetOrLoad(textureDir / sec.NormalTexturePath);
            }
        }
    }
}