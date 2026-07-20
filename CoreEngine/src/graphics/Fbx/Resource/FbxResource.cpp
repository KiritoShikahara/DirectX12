#include "pch.h"
#include "FbxResource.h"

#include <graphics/Texture/Texture.h>
#include<graphics/Texture/TextureManager.h>
#include <system/AssetPath/AssetPathManager.h>

namespace graphics
{

    // ============================================================
    //  Load  (.bin のみ)
    // ============================================================
    bool FbxResource::Load(
        const std::string& binPath)
    {
        if (!LoadBin(binPath)) return false;

        mIsLoaded = true;
        DEBUG_LOG(sys::eLogLevel::Log, std::format("FbxResource: Loaded '{}'", binPath));
        return true;
    }

    // ============================================================
    //  LoadBin  (.bin パーサ)
    //
    //  フォーマット (FbxAnalyzer の fwrite 順と完全一致):
    //    [MeshCount:i32][PolygonCount:i32][VertexCount:i32]
    //    Vertices × VertexCount
    //      Position:vec3  UV:vec2  Normal:vec3  Tangent:vec3
    //      Bone:i32[4]   Weight:f32[4]
    //    [IndexCount:i32]  Indices × IndexCount
    //    [MaterialCount:i32]
    //      Material:
    //        [NameSize:i32][Name]
    //        [AlbedoSize:i32][AlbedoPath]  ... × 6チャンネル
    //        BaseColorFactor:vec3  MetallicFactor:f32  RoughnessFactor:f32
    //        EmissiveFactor:vec3
    //        PolygonCount:u32
    //    [BoneCount:i32]
    //      Bone: [NameSize:i32][Name]  ParentIndex:i32  BindMatrix:Matrix
    // ============================================================
    bool FbxResource::LoadBin(const std::string& binPath)
    {
        FILE* fp = nullptr;
        if (fopen_s(&fp, binPath.c_str(), "rb") != 0)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                std::format("FbxResource: Cannot open '{}'", binPath));
            return false;
        }

        auto& texManager = graphics::TextureManager::Get();


        // テクスチャは binPath の隣の "Texture" フォルダを基準とする
        const std::filesystem::path baseDir =
            std::filesystem::path(binPath).parent_path() / "Texture";

        auto ReadStr = [&](std::string& s)
            {
                int32_t size = 0;
                fread(&size, sizeof(int32_t), 1, fp);
                s.resize(size);
                if (size > 0) fread(s.data(), 1, size, fp);
            };

        // isSRGB: Albedo/EmissiveはsRGBエンコードされた色情報のため、
        // SRVをsRGBとして解釈し線形空間で正しくライティング計算できるようにする。
        // Normal/Metallic/Roughness/AOは非色データのため線形のまま扱う。
        auto LoadTex = [&](const std::string& relPath, bool isSRGB) -> Texture*
            {
                if (relPath.empty()) return nullptr;
                return texManager.GetOrLoad((baseDir / relPath).string(), isSRGB);
            };

        // ---- ヘッダー ----
        int32_t meshCount = 0, polyCount = 0, vertexCount = 0;
        fread(&meshCount, sizeof(int32_t), 1, fp);
        fread(&polyCount, sizeof(int32_t), 1, fp);
        fread(&vertexCount, sizeof(int32_t), 1, fp);

        // ---- 頂点 ----
        std::vector<FbxVertex> vertices(vertexCount);
        fread(vertices.data(), sizeof(FbxVertex), vertexCount, fp);
        {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;
            for (const auto& v : vertices)
            {
                minX = std::min(minX, v.Position.x); maxX = std::max(maxX, v.Position.x);
                minY = std::min(minY, v.Position.y); maxY = std::max(maxY, v.Position.y);
                minZ = std::min(minZ, v.Position.z); maxZ = std::max(maxZ, v.Position.z);
            }
            if (vertexCount > 0)
            {
                // 底面(minY)が y=0 になり、XZ が中心になるオフセット
                mBottomCenterPivot =
                {
                    -(minX + maxX) * 0.5f,
                    -minY,
                    -(minZ + maxZ) * 0.5f
                };
            }
        }

        // ---- インデックス ----
        int32_t indexCount = 0;
        fread(&indexCount, sizeof(int32_t), 1, fp);
        std::vector<uint32_t> indices(indexCount);
        fread(indices.data(), sizeof(uint32_t), indexCount, fp);

        // ---- マテリアル → FbxSection ----
        int32_t materialCount = 0;
        fread(&materialCount, sizeof(int32_t), 1, fp);
        mSections.reserve(materialCount);

        uint32_t indexOffset = 0;
        for (int i = 0; i < materialCount; ++i)
        {
            FbxSection sec = {};
            std::string albedoPath, normalPath, metallicPath,
                roughnessPath, aoPath, emissivePath;

            ReadStr(sec.Name);
            ReadStr(albedoPath);
            ReadStr(normalPath);
            ReadStr(metallicPath);
            ReadStr(roughnessPath);
            ReadStr(aoPath);
            ReadStr(emissivePath);

            sec.AlbedoTexture = LoadTex(albedoPath, true);
            sec.NormalTexture = LoadTex(normalPath, false);
            sec.MetallicTexture = LoadTex(metallicPath, false);
            sec.RoughnessTexture = LoadTex(roughnessPath, false);
            sec.AOTexture = LoadTex(aoPath, false);
            sec.EmissiveTexture = LoadTex(emissivePath, true);

            DirectX::XMFLOAT3 baseColor = {};
            fread(&baseColor, sizeof(DirectX::XMFLOAT3), 1, fp);
            fread(&sec.MetallicFactor, sizeof(float), 1, fp);
            fread(&sec.RoughnessFactor, sizeof(float), 1, fp);
            fread(&sec.EmissiveFactor, sizeof(DirectX::XMFLOAT3), 1, fp);
            sec.BaseColorFactor = baseColor;

            uint32_t matPolyCount = 0;
            fread(&matPolyCount, sizeof(uint32_t), 1, fp);
            sec.IndexCount = matPolyCount * 3;
            sec.IndexOffset = indexOffset;
            indexOffset += sec.IndexCount;

            mSections.push_back(std::move(sec));
        }

        // ---- ボーン ----
        int32_t boneCount = 0;
        fread(&boneCount, sizeof(int32_t), 1, fp);
        mBones.reserve(boneCount);

        for (int i = 0; i < boneCount; ++i)
        {
            FbxBoneData bone = {};
            ReadStr(bone.Name);
            fread(&bone.ParentIndex, sizeof(int32_t), 1, fp);
            fread(&bone.BindMatrix, sizeof(DirectX::XMFLOAT4X4), 1, fp);
            // LocalTransform はレストポーズとして BindMatrix の逆行列で近似
            // (FBX .bin には保存されないため単位行列で初期化)
            DirectX::XMStoreFloat4x4(&bone.LocalTransform,
                DirectX::XMMatrixIdentity());
            mBones.push_back(std::move(bone));
        }

        fclose(fp);

        // ---- GPU バッファ構築 (CreateStaticSync = cmdList 不要) ----
        mVB = std::make_unique<VertexBuffer>();
        if (!mVB->CreateStaticSync(
            vertices.data(),
            sizeof(FbxVertex) * vertexCount,
            sizeof(FbxVertex)))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxResource: Failed to create vertex buffer.");
            return false;
        }

        mIB = std::make_unique<IndexBuffer>();
        if (!mIB->CreateStaticSync(
            indices.data(),
            sizeof(uint32_t) * indexCount,
            DXGI_FORMAT_R32_UINT))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "FbxResource: Failed to create index buffer.");
            return false;
        }

        return true;
    }

    // ============================================================
    //  LoadAnm  (.anm の追加ロード)
    //  複数回呼べるので1モデルに複数アニメーションを持てる
    //  clipName を省略するとファイル名(拡張子なし)をクリップ名にする
    // ============================================================
    bool FbxResource::LoadAnm(const std::string& anmPath, const std::string& clipName)
    {
        FILE* fp = nullptr;
        if (fopen_s(&fp, anmPath.c_str(), "rb") != 0)
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                std::format("FbxResource: Cannot open anm '{}'", anmPath));
            return false;
        }

        FbxAnimClip clip = {};
        // clipName 未指定ならファイル名(拡張子なし)をクリップ名にする
        clip.Name = clipName.empty()
            ? std::filesystem::path(anmPath).stem().string()
            : clipName;

        fread(&clip.NumFrame, sizeof(int32_t), 1, fp);
        clip.Duration = clip.NumFrame / clip.FrameRate;

        int32_t numBone = 0;
        fread(&numBone, sizeof(int32_t), 1, fp);
        clip.KeyFrames.resize(numBone);

        for (int b = 0; b < numBone; ++b)
        {
            int32_t frameCount = 0;
            fread(&frameCount, sizeof(int32_t), 1, fp);
            clip.KeyFrames[b].resize(frameCount);
            fread(clip.KeyFrames[b].data(),
                sizeof(DirectX::XMFLOAT4X4), frameCount, fp);
        }

        fclose(fp);

        // 実行時のXMMatrixDecomposeを無くすため、ここで一度だけTRSへ分解しておく。
        // 分解結果はエンティティに依存しないため、同じモデルを何体表示しても再利用できる
        clip.KeyFrameTrs.resize(clip.KeyFrames.size());
        for (size_t b = 0; b < clip.KeyFrames.size(); ++b)
        {
            const auto& track = clip.KeyFrames[b];
            auto& trsTrack = clip.KeyFrameTrs[b];
            trsTrack.resize(track.size());

            for (size_t f = 0; f < track.size(); ++f)
            {
                DirectX::XMVECTOR scale, rotation, translation;
                if (DirectX::XMMatrixDecompose(&scale, &rotation, &translation,
                    DirectX::XMLoadFloat4x4(&track[f])))
                {
                    DirectX::XMStoreFloat4(&trsTrack[f].Scale, scale);
                    DirectX::XMStoreFloat4(&trsTrack[f].Rotation, rotation);
                    DirectX::XMStoreFloat4(&trsTrack[f].Translation, translation);
                }
                // 分解に失敗した場合(退化した行列など)は既定値(単位変換)のままにする
            }
        }

        mAnimClips.push_back(std::move(clip));
        return true;
    }

    // ============================================================
    //  FindClipIndex
    // ============================================================
    int FbxResource::FindClipIndex(const std::string& name) const
    {
        for (int i = 0; i < static_cast<int>(mAnimClips.size()); ++i)
        {
            if (mAnimClips[i].Name == name) return i;
        }
        return -1;
    }

    bool FbxResource::BuildFromMemory(const std::vector<FbxVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<FbxSection>& sections)
    {
        if (vertices.empty() || indices.empty())
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "FbxResource::BuildFromMemory: empty vertices or indices.");
            return false;
        }

        mSections = sections;
        // mBones / mAnimClips は空のまま (スキニングなし)

        const uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
        const uint32_t indexCount = static_cast<uint32_t>(indices.size());

        mVB = std::make_unique<VertexBuffer>();
        if (!mVB->CreateStaticSync(
            vertices.data(),
            sizeof(FbxVertex) * vertexCount,
            sizeof(FbxVertex)))
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "FbxResource::BuildFromMemory: Failed to create vertex buffer.");
            return false;
        }

        mIB = std::make_unique<IndexBuffer>();
        if (!mIB->CreateStaticSync(
            indices.data(),
            sizeof(uint32_t) * indexCount,
            DXGI_FORMAT_R32_UINT))
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "FbxResource::BuildFromMemory: Failed to create index buffer.");
            return false;
        }

        mIsLoaded = true;
        DEBUG_LOG(sys::eLogLevel::Log,
            std::format("FbxResource::BuildFromMemory: {} verts, {} indices, {} sections.",
                vertexCount, indexCount, sections.size()));
        return true;
    }

    // ============================================================
    //  SetBuffers
    // ============================================================
    void FbxResource::SetBuffers(ID3D12GraphicsCommandList* cmdList) const
    {
        if (mVB) mVB->Set(cmdList);
        if (mIB) mIB->Set(cmdList);
    }

} // namespace graphics