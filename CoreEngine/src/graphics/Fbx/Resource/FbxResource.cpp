#include "pch.h"
#include "FbxResource.h"

#include <graphics/Texture/Texture.h>
#include <graphics/Texture/TextureManager.h>
#include <system/AssetPath/AssetPathManager.h>

namespace graphics
{
	// FBX モデルの読み込み
	bool FbxResource::Load(
		const std::string& binPath)
	{
		if (!LoadBin(binPath)) return false;

		mIsLoaded = true;
		DEBUG_LOG(sys::eLogLevel::Log, std::format("FbxResource: Loaded '{}'", binPath));
		return true;
	}

	// バイナリデータのパース処理
	FbxResource::LoadedBinData FbxResource::LoadBinData(const std::string& binPath)
	{
		LoadedBinData result;

		FILE* fp = nullptr;
		if (fopen_s(&fp, binPath.c_str(), "rb") != 0)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				std::format("FbxResource: Cannot open '{}'", binPath));
			return result;
		}

		result.TextureBaseDir = std::filesystem::path(binPath).parent_path() / "Texture";

		auto ReadStr = [&](std::string& s)
			{
				int32_t size = 0;
				fread(&size, sizeof(int32_t), 1, fp);
				s.resize(size);
				if (size > 0) fread(s.data(), 1, size, fp);
			};

		int32_t meshCount = 0, polyCount = 0, vertexCount = 0;
		fread(&meshCount, sizeof(int32_t), 1, fp);
		fread(&polyCount, sizeof(int32_t), 1, fp);
		fread(&vertexCount, sizeof(int32_t), 1, fp);

		result.Vertices.resize(vertexCount);
		fread(result.Vertices.data(), sizeof(FbxVertex), vertexCount, fp);
		{
			float minX = FLT_MAX, maxX = -FLT_MAX;
			float minY = FLT_MAX, maxY = -FLT_MAX;
			float minZ = FLT_MAX, maxZ = -FLT_MAX;
			for (const auto& v : result.Vertices)
			{
				minX = std::min(minX, v.Position.x); maxX = std::max(maxX, v.Position.x);
				minY = std::min(minY, v.Position.y); maxY = std::max(maxY, v.Position.y);
				minZ = std::min(minZ, v.Position.z); maxZ = std::max(maxZ, v.Position.z);
			}
			if (vertexCount > 0)
			{
				result.BottomCenterPivot =
				{
					-(minX + maxX) * 0.5f,
					-minY,
					-(minZ + maxZ) * 0.5f
				};
			}
		}

		int32_t indexCount = 0;
		fread(&indexCount, sizeof(int32_t), 1, fp);
		result.Indices.resize(indexCount);
		fread(result.Indices.data(), sizeof(uint32_t), indexCount, fp);

		int32_t materialCount = 0;
		fread(&materialCount, sizeof(int32_t), 1, fp);
		result.Materials.reserve(materialCount);

		uint32_t indexOffset = 0;
		for (int i = 0; i < materialCount; ++i)
		{
			MaterialInfo mat;

			ReadStr(mat.Name);
			ReadStr(mat.AlbedoPath);
			ReadStr(mat.NormalPath);
			ReadStr(mat.MetallicPath);
			ReadStr(mat.RoughnessPath);
			ReadStr(mat.AOPath);
			ReadStr(mat.EmissivePath);

			DirectX::XMFLOAT3 baseColor = {};
			fread(&baseColor, sizeof(DirectX::XMFLOAT3), 1, fp);
			fread(&mat.MetallicFactor, sizeof(float), 1, fp);
			fread(&mat.RoughnessFactor, sizeof(float), 1, fp);
			fread(&mat.EmissiveFactor, sizeof(DirectX::XMFLOAT3), 1, fp);
			mat.BaseColorFactor = baseColor;

			uint32_t matPolyCount = 0;
			fread(&matPolyCount, sizeof(uint32_t), 1, fp);
			mat.IndexCount = matPolyCount * 3;
			mat.IndexOffset = indexOffset;
			indexOffset += mat.IndexCount;

			result.Materials.push_back(std::move(mat));
		}

		int32_t boneCount = 0;
		fread(&boneCount, sizeof(int32_t), 1, fp);
		result.Bones.reserve(boneCount);

		for (int i = 0; i < boneCount; ++i)
		{
			FbxBoneData bone = {};
			ReadStr(bone.Name);
			fread(&bone.ParentIndex, sizeof(int32_t), 1, fp);
			fread(&bone.BindMatrix, sizeof(DirectX::XMFLOAT4X4), 1, fp);
			DirectX::XMStoreFloat4x4(&bone.LocalTransform,
				DirectX::XMMatrixIdentity());
			result.Bones.push_back(std::move(bone));
		}

		fclose(fp);

		result.Success = true;
		return result;
	}

	// テクスチャパスの収集処理
	void FbxResource::CollectTexturePaths(
		const LoadedBinData& data,
		std::vector<std::filesystem::path>& outSrgbPaths,
		std::vector<std::filesystem::path>& outLinearPaths)
	{
		auto add = [](std::vector<std::filesystem::path>& out, const std::filesystem::path& baseDir, const std::string& relPath)
			{
				if (!relPath.empty()) out.push_back(baseDir / relPath);
			};

		for (const auto& mat : data.Materials)
		{
			add(outSrgbPaths, data.TextureBaseDir, mat.AlbedoPath);
			add(outSrgbPaths, data.TextureBaseDir, mat.EmissivePath);
			add(outLinearPaths, data.TextureBaseDir, mat.NormalPath);
			add(outLinearPaths, data.TextureBaseDir, mat.MetallicPath);
			add(outLinearPaths, data.TextureBaseDir, mat.RoughnessPath);
			add(outLinearPaths, data.TextureBaseDir, mat.AOPath);
		}
	}

	// バイナリデータから GPU リソースを構築する
	bool FbxResource::CreateFromBinData(const LoadedBinData& data)
	{
		if (!data.Success) return false;

		auto& texManager = graphics::TextureManager::Get();

		auto LoadTex = [&](const std::string& relPath, bool isSRGB) -> Texture*
			{
				if (relPath.empty()) return nullptr;
				return texManager.GetOrLoad((data.TextureBaseDir / relPath).string(), isSRGB);
			};

		mSections.clear();
		mSections.reserve(data.Materials.size());
		for (const auto& mat : data.Materials)
		{
			FbxSection sec = {};
			sec.Name = mat.Name;
			sec.AlbedoTexture = LoadTex(mat.AlbedoPath, true);
			sec.NormalTexture = LoadTex(mat.NormalPath, false);
			sec.MetallicTexture = LoadTex(mat.MetallicPath, false);
			sec.RoughnessTexture = LoadTex(mat.RoughnessPath, false);
			sec.AOTexture = LoadTex(mat.AOPath, false);
			sec.EmissiveTexture = LoadTex(mat.EmissivePath, true);
			sec.BaseColorFactor = mat.BaseColorFactor;
			sec.MetallicFactor = mat.MetallicFactor;
			sec.RoughnessFactor = mat.RoughnessFactor;
			sec.EmissiveFactor = mat.EmissiveFactor;
			sec.IndexCount = mat.IndexCount;
			sec.IndexOffset = mat.IndexOffset;

			mSections.push_back(std::move(sec));
		}

		mBones = data.Bones;
		mBottomCenterPivot = data.BottomCenterPivot;

		const uint32_t vertexCount = static_cast<uint32_t>(data.Vertices.size());
		const uint32_t indexCount = static_cast<uint32_t>(data.Indices.size());

		mVB = std::make_unique<VertexBuffer>();
		if (!mVB->CreateStaticSync(
			data.Vertices.data(),
			sizeof(FbxVertex) * vertexCount,
			sizeof(FbxVertex)))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "FbxResource: Failed to create vertex buffer.");
			return false;
		}

		mIB = std::make_unique<IndexBuffer>();
		if (!mIB->CreateStaticSync(
			data.Indices.data(),
			sizeof(uint32_t) * indexCount,
			DXGI_FORMAT_R32_UINT))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "FbxResource: Failed to create index buffer.");
			return false;
		}

		mIsLoaded = true;
		return true;
	}

	// バイナリファイルの内部ロード処理
	bool FbxResource::LoadBin(const std::string& binPath)
	{
		const LoadedBinData data = LoadBinData(binPath);
		return CreateFromBinData(data);
	}

	// アニメーションファイルの追加ロード処理
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
			}
		}

		mAnimClips.push_back(std::move(clip));
		return true;
	}

	// クリップ名からインデックスを取得する
	int FbxResource::FindClipIndex(const std::string& name) const
	{
		for (int i = 0; i < static_cast<int>(mAnimClips.size()); ++i)
		{
			if (mAnimClips[i].Name == name) return i;
		}
		return -1;
	}

	// メモリ上のデータから直接構築する
	bool FbxResource::BuildFromMemory(const std::vector<FbxVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<FbxSection>& sections)
	{
		if (vertices.empty() || indices.empty())
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"FbxResource::BuildFromMemory: empty vertices or indices.");
			return false;
		}

		mSections = sections;

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

	// バッファをコマンドリストにセットする
	void FbxResource::SetBuffers(ID3D12GraphicsCommandList* cmdList) const
	{
		if (mVB) mVB->Set(cmdList);
		if (mIB) mIB->Set(cmdList);
	}

} // namespace graphics