#include"pch.h"
#include "ModelResouce.h"

#include<graphics/Model/Formats/BinFormat.h>
#include<graphics/Model/Formats/AnmFormat.h>
#include<graphics/Texture/Texture.h>
#include<graphics/Texture/TextureManager.h>

namespace graphics
{

    bool ModelResource::Load(const std::string& binPath)
    {
        std::vector<ModelVertex> verts;
        std::vector<uint32_t>    indices;

        if (!LoadBin(binPath, verts, indices)) return false;

        if (!UploadGPU(verts, indices)) return false;

        ResolveTextures(std::filesystem::path(binPath).parent_path());
        ResolveBoneIndices();

        mIsLoaded = true;
        DEBUG_LOG(sys::eLogLevel::Log,
            std::format("ModelResource: Loaded '{}' ({} sections, {} bones, {} clips)",
                binPath, mSections.size(), mBones.size(), mAnimClips.size()));
        return true;
    }

    /// <summary>
    /// .anm を追加読み込みしてクリップを追記する。
    /// 同じリソースに何度でも呼べる（クリップが末尾に追加される）。
    /// </summary>
    bool ModelResource::AppendAnimation(const std::string& anmPath)
    {
        if (!mIsLoaded)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "ModelResource::AppendAnimation: リソースが未ロードです。");
            return false;
        }
        if (!LoadAnm(anmPath))
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                std::format("ModelResource::AppendAnimation: Failed: {}", anmPath));
            return false;
        }
        ResolveBoneIndices();
        DEBUG_LOG(sys::eLogLevel::Log,
            std::format("ModelResource::AppendAnimation: Loaded '{}' (total {} clips)",
                anmPath, mAnimClips.size()));
        return true;
    }

    int ModelResource::FindClipIndex(const std::string& clipName) const
    {
        for (int i = 0; i < static_cast<int>(mAnimClips.size()); ++i)
        {
            if (mAnimClips[i].Name == clipName) return i;
        }
        return -1;
    }

    /// <summary>
    /// .binからCPUバッファ
    /// </summary>
    /// <returns></returns>
    bool ModelResource::LoadBin(const std::string& path, std::vector<ModelVertex>& outVerts, std::vector<uint32_t>& outIndices)
    {
        FILE* fp = nullptr;
        if (fopen_s(&fp, path.c_str(), "rb") != 0 || !fp)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                std::format("ModelResource: File not found: {}", path));
            return false;
        }

        // ヘッダー
        BinFmt::BinHeader header{};
        fread(&header, sizeof(header), 1, fp);

        if (std::memcmp(header.magic, BinFmt::MAGIC, 4) != 0)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                std::format("ModelResource: Invalid magic: {}", path));
            fclose(fp); return false;
        }
        if (header.version != BinFmt::VERSION)
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                std::format("ModelResource: Version mismatch (file={}, expected={})",
                    header.version, BinFmt::VERSION));
        }

        // マテリアル
        struct MatInfo
        {
            std::string name, diffuse, normal, specular, emissive;
            DirectX::XMFLOAT4 baseColor;
            float metallic, roughness;
            uint32_t flags;
        };
        std::vector<MatInfo> matInfos(header.materialCount);

        for (uint32_t i = 0; i < header.materialCount; ++i)
        {
            BinFmt::MaterialEntry e{};
            fread(&e, sizeof(e), 1, fp);
            auto& m = matInfos[i];
            m.name = e.name;
            m.diffuse = e.diffuseTex;
            m.normal = e.normalTex;
            m.specular = e.specularTex;
            m.emissive = e.emissiveTex;
            m.baseColor = { e.baseColor.x, e.baseColor.y, e.baseColor.z, e.baseColor.w };
            m.metallic = e.metallic;
            m.roughness = e.roughness;
            m.flags = e.flags;
        }

        // ボーン
        mBones.resize(header.boneCount);
        for (uint32_t i = 0; i < header.boneCount; ++i)
        {
            BinFmt::BoneEntry e{};
            fread(&e, sizeof(e), 1, fp);
            auto& b = mBones[i];
            b.Name = e.name;
            b.ParentIndex = e.parentIndex;
            // BinFmt::Mat4x4 と XMFLOAT4X4 は同じメモリレイアウト (float[4][4])
            std::memcpy(&b.OffsetMatrix, &e.offsetMatrix, sizeof(DirectX::XMFLOAT4X4));
            std::memcpy(&b.LocalTransform, &e.localTransform, sizeof(DirectX::XMFLOAT4X4));
        }

        // メッシュ
        uint32_t vertexCursor = 0;
        uint32_t indexCursor = 0;

        for (uint32_t mi = 0; mi < header.meshCount; ++mi)
        {
            BinFmt::MeshEntry meshEntry{};
            fread(&meshEntry, sizeof(meshEntry), 1, fp);

            // 頂点: BinFmt::Vertex と ModelVertex は同じレイアウト → 直接読み込み
            const uint32_t vc = meshEntry.vertexCount;
            const size_t   prevVertSize = outVerts.size();
            outVerts.resize(prevVertSize + vc);
            fread(outVerts.data() + prevVertSize, sizeof(ModelVertex), vc, fp);

            // インデックス: 16 or 32 bit → 常に 32bit に変換し vertexCursor でリベース
            const uint32_t ic = meshEntry.indexCount;
            if (meshEntry.use32BitIndex)
            {
                std::vector<uint32_t> raw(ic);
                fread(raw.data(), sizeof(uint32_t), ic, fp);
                for (auto idx : raw) outIndices.push_back(idx + vertexCursor);
            }
            else
            {
                std::vector<uint16_t> raw(ic);
                fread(raw.data(), sizeof(uint16_t), ic, fp);
                for (auto idx : raw) outIndices.push_back(static_cast<uint32_t>(idx) + vertexCursor);
            }

            // セクション生成
            ModelSection sec{};
            sec.MeshName = meshEntry.name;
            sec.IndexOffset = indexCursor;
            sec.IndexCount = ic;
            if (meshEntry.materialIndex < header.materialCount)
            {
                const auto& mat = matInfos[meshEntry.materialIndex];
                sec.MaterialName = mat.name;
                sec.DiffuseTexPath = mat.diffuse;
                sec.NormalTexPath = mat.normal;
                sec.SpecularTexPath = mat.specular;
                sec.EmissiveTexPath = mat.emissive;
                sec.BaseColor = mat.baseColor;
                sec.Metallic = mat.metallic;
                sec.Roughness = mat.roughness;
                sec.Flags = mat.flags;
            }
            mSections.push_back(sec);

            vertexCursor += vc;
            indexCursor += ic;
        }

        fclose(fp);
        return true;
    }

    /// <summary>
    /// .anm -> ModelAnimClip
    /// </summary>
    bool ModelResource::LoadAnm(const std::string& path)
    {
        FILE* fp = nullptr;
        if (fopen_s(&fp, path.c_str(), "rb") != 0 || !fp) return false;

        AnmFmt::AnmHeader header{};
        fread(&header, sizeof(header), 1, fp);

        if (std::memcmp(header.magic, AnmFmt::MAGIC, 4) != 0)
        {
            DEBUG_LOG(sys::eLogLevel::Error, "ModelResource: Invalid .anm magic.");
            fclose(fp); return false;
        }

        mAnimClips.resize(header.animationCount);

        for (uint32_t ai = 0; ai < header.animationCount; ++ai)
        {
            AnmFmt::AnimEntry animEntry{};
            fread(&animEntry, sizeof(animEntry), 1, fp);

            auto& clip = mAnimClips[ai];
            clip.Name = animEntry.name;
            clip.Duration = animEntry.duration;
            clip.IsBaked = (animEntry.isBaked == 1u);
            clip.BakeFrameRate = animEntry.bakeFrameRate;

            if (clip.IsBaked)
            {
                clip.BakedTracks.resize(animEntry.channelCount);
                for (uint32_t ci = 0; ci < animEntry.channelCount; ++ci)
                {
                    AnmFmt::BakedChannelEntry ce{};
                    fread(&ce, sizeof(ce), 1, fp);

                    auto& track = clip.BakedTracks[ci];
                    track.BoneName = ce.boneName;
                    track.Frames.resize(ce.frameCount);
                    // ModelBakedFrame と AnmFmt::BakedFrame は同一レイアウト (40 bytes)
                    fread(track.Frames.data(), sizeof(ModelBakedFrame), ce.frameCount, fp);
                }
            }
            else
            {
                clip.SparseTracks.resize(animEntry.channelCount);
                for (uint32_t ci = 0; ci < animEntry.channelCount; ++ci)
                {
                    AnmFmt::ChannelEntry ce{};
                    fread(&ce, sizeof(ce), 1, fp);

                    auto& track = clip.SparseTracks[ci];
                    track.BoneName = ce.boneName;
                    track.PosKeys.resize(ce.posKeyCount);
                    track.RotKeys.resize(ce.rotKeyCount);
                    track.ScaleKeys.resize(ce.scaleKeyCount);
                    // ModelPosKey/RotKey/ScaleKey と AnmFmt 版は同一レイアウト
                    fread(track.PosKeys.data(), sizeof(ModelPosKey), ce.posKeyCount, fp);
                    fread(track.RotKeys.data(), sizeof(ModelRotKey), ce.rotKeyCount, fp);
                    fread(track.ScaleKeys.data(), sizeof(ModelScaleKey), ce.scaleKeyCount, fp);
                }
            }
        }

        fclose(fp);
        return true;
    }

    bool ModelResource::UploadGPU(const std::vector<ModelVertex>& verts, const std::vector<uint32_t>& indices)
    {
        mVB = std::make_unique<VertexBuffer>();
        const uint32_t vbSize = static_cast<uint32_t>(verts.size() * sizeof(ModelVertex));
        if (!mVB->CreateStaticSync(verts.data(), vbSize, sizeof(ModelVertex)))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "ModelResource: Failed to create vertex buffer.");
            return false;
        }

        mIB = std::make_unique<IndexBuffer>();
        const uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
        if (!mIB->CreateStaticSync(indices.data(), ibSize, DXGI_FORMAT_R32_UINT))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "ModelResource: Failed to create index buffer.");
            return false;
        }

        return true;
    }

    void ModelResource::ResolveTextures(const std::filesystem::path& baseDir)
    {
        auto& texMgr = graphics::TextureManager::Get();

        for (auto& sec : mSections)
        {
            if (!sec.DiffuseTexPath.empty())
            {
                sec.DiffuseTexture = texMgr.GetOrLoad(baseDir / sec.DiffuseTexPath);
                if (!sec.DiffuseTexture)
                    DEBUG_LOG(sys::eLogLevel::Warning,
                        std::format("ModelResource: Diffuse texture not found: {}", sec.DiffuseTexPath));
            }
            if (!sec.NormalTexPath.empty())
                sec.NormalTexture = texMgr.GetOrLoad(baseDir / sec.NormalTexPath);

            if (!sec.SpecularTexPath.empty())
                sec.SpecularTexture = texMgr.GetOrLoad(baseDir / sec.SpecularTexPath);
        }
    }

    void ModelResource::ResolveBoneIndices()
    {
        if (mBones.empty() || mAnimClips.empty()) return;

        // 名前 → インデックス マップを構築
        std::unordered_map<std::string, int32_t> nameMap;
        nameMap.reserve(mBones.size());
        for (int32_t i = 0; i < static_cast<int32_t>(mBones.size()); ++i)
            nameMap[mBones[i].Name] = i;

        for (auto& clip : mAnimClips)
        {
            if (clip.IsBaked)
            {
                for (auto& track : clip.BakedTracks)
                {
                    auto it = nameMap.find(track.BoneName);
                    track.BoneIndex = (it != nameMap.end()) ? it->second : -1;
                }
            }
            else
            {
                for (auto& track : clip.SparseTracks)
                {
                    auto it = nameMap.find(track.BoneName);
                    track.BoneIndex = (it != nameMap.end()) ? it->second : -1;
                }
            }
        }
    }

}

