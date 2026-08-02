#include "pch.h"
#include"Texture.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>

namespace fs = ::std::filesystem;
namespace DirectXTex = ::DirectX;

namespace graphics
{
	Texture::Texture()
		: mWidth(0)
		, mHeight(0)
		, mResource(nullptr)
		, mAllocation(nullptr)
		, mSrvHeap()
	{
	}

	Texture::~Texture()
	{
		Release();
	}
	bool Texture::Create(const std::filesystem::path& FilePath, bool isSRGB)
	{
 
		const ImageData imageData = LoadImageData(FilePath, isSRGB);
		if (!imageData.Success) return false;

		return CreateFromImageData(FilePath, isSRGB, imageData);
	}

	// CPU側の処理
	Texture::ImageData Texture::LoadImageData(const std::filesystem::path& FilePath, bool isSRGB)
	{
		ImageData result;

		if (FilePath.empty() || FilePath.string().find_first_not_of(" \t\r\n") == std::string::npos)
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Texture: FilePath is empty or invalid.");
			return result;
		}

		if (fs::exists(FilePath) == false || fs::is_regular_file(FilePath) == false)
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Texture: File not found or is not a regular file: {}", FilePath.string());
			return result;
		}

		//	拡張子
		const fs::path ext = FilePath.extension();
		//	ファイルパス
		std::wstring path = FilePath.wstring();
		HRESULT hr = S_FALSE;
		//	テクスチャのメタデータ
		DirectX::TexMetadata metaData = {};
		DirectX::ScratchImage scratchImage = {};
		//	大文字、小文字を無視して判定をする。
		if (_wcsicmp(ext.c_str(), L".dds") == 0)
		{
			hr = DirectXTex::LoadFromDDSFile(
				path.c_str(),
				DirectXTex::DDS_FLAGS_NONE,
				&metaData,
				scratchImage);
		}
		else if (_wcsicmp(ext.c_str(), L".tga") == 0)
		{
			hr = DirectXTex::LoadFromTGAFile(
				path.c_str(),
				DirectXTex::TGA_FLAGS_NONE,
				&metaData,
				scratchImage);
		}
		else if (_wcsicmp(ext.c_str(), L".hdr") == 0)
		{
			hr = DirectXTex::LoadFromHDRFile(
				path.c_str(),
				&metaData,
				scratchImage);
		}
		else
		{
			hr = DirectXTex::LoadFromWICFile(
				path.c_str(),
				DirectXTex::WIC_FLAGS_NONE,
				&metaData,
				scratchImage);

		}
		if (FAILED(hr))
		{
			return result;
		}

		// ミップマップがない画像は生成
		// ちらつき防止
		if (metaData.mipLevels <= 1 && metaData.IsVolumemap() == false)
		{
			// sRGB用テクスチャはガンマ空間のまま縮小フィルタすると暗部が変色するため、
			// 線形空間に変換してからフィルタするフラグを付ける。
			const DirectXTex::TEX_FILTER_FLAGS filterFlags = isSRGB
				? DirectXTex::TEX_FILTER_SRGB
				: DirectXTex::TEX_FILTER_DEFAULT;

			DirectX::ScratchImage mipChain;
			HRESULT mipHr = DirectXTex::GenerateMipMaps(
				scratchImage.GetImages(), scratchImage.GetImageCount(), metaData,
				filterFlags, 0, mipChain);

			if (SUCCEEDED(mipHr))
			{
				scratchImage = std::move(mipChain);
				metaData = scratchImage.GetMetadata();
			}
			else
			{
				DEBUG_LOG(sys::eLogLevel::Warning,
					"Texture: Failed to generate mipmaps (using single mip level): {}", FilePath.string());
			}
		}

		result.Success = true;
		result.MetaData = metaData;
		result.ScratchImage = std::move(scratchImage);
		return result;
	}

	// GPU側の処理
	bool Texture::CreateFromImageData(const std::filesystem::path& FilePath, bool isSRGB, const ImageData& imageData)
	{
		if (!imageData.Success) return false;

		const DirectX::TexMetadata& metaData = imageData.MetaData;
		const DirectX::ScratchImage& scratchImage = imageData.ScratchImage;

		HRESULT hr = S_FALSE;
		auto& DX12Device = graphics::DX12Device::Get();
		auto device = DX12Device.GetDevice();
		auto allocator = DX12Device.GetMAAllocator();
		auto& heapManager = graphics::GDescriptorHeapManager::Get();

		// GPUリソースの作成
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension);
		resDesc.Format = metaData.format;
		resDesc.Width = static_cast<UINT64>(metaData.width);
		resDesc.Height = static_cast<UINT>(metaData.height);
		resDesc.DepthOrArraySize = static_cast<UINT16>(metaData.arraySize);
		resDesc.MipLevels = static_cast<UINT16>(metaData.mipLevels);
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		/// リソース作成
		hr = allocator->CreateResource(
			&allocDesc,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,   // アップロード前は転送先状態
			nullptr,
			&mAllocation,
			IID_PPV_ARGS(&mResource));

		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Texture: Failed to create GPU resource for texture: {}", FilePath.string());
			return false;
		}

		// サブリソースデータの構築
		const UINT numSubresources =
			static_cast<UINT>(metaData.mipLevels * metaData.arraySize);

		std::vector<D3D12_SUBRESOURCE_DATA> subresources(numSubresources);
		for (UINT i = 0; i < numSubresources; ++i)
		{
			const UINT mip = i % static_cast<UINT>(metaData.mipLevels);
			const UINT arraySlice = i / static_cast<UINT>(metaData.mipLevels);

			const DirectXTex::Image* img = scratchImage.GetImage(mip, arraySlice, 0);
			subresources[i].pData = img->pixels;
			subresources[i].RowPitch = static_cast<LONG_PTR>(img->rowPitch);
			subresources[i].SlicePitch = static_cast<LONG_PTR>(img->slicePitch);
		}

		// VRAMへのアップロード
		if (!DX12Device.UploadTextureData(mResource.Get(), subresources))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Texture: Failed to upload texture data to GPU: {}", FilePath.string());
			return false;
		}

		// SRVの作成
		// isSRGB指定時はGPUリソース自体はUNORMのまま、SRVの解釈のみをSRGBにする。
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = isSRGB ? DirectXTex::MakeSRGB(metaData.format) : metaData.format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (metaData.IsCubemap())
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = static_cast<UINT>(metaData.mipLevels);
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = static_cast<UINT>(metaData.mipLevels);
		}

		// GDescriptorHeapInfoにSRVを割り当てる
		if (!mSrvHeap.Create(heapManager, 1))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"Texture: Failed to allocate SRV descriptor slot. path={}", FilePath.string());
			return false;
		}

		device->CreateShaderResourceView(mResource.Get(), &srvDesc, mSrvHeap.GetCpuHandle());

		// サイズ保存
		mWidth = static_cast<float>(metaData.width);
		mHeight = static_cast<float>(metaData.height);

		return true;
	}

	/// <summary>
	/// リソースの解放
	/// </summary>
	void Texture::Release()
	{
		mSrvHeap.Release();

		mAllocation.Reset();
		mResource.Reset();

		mWidth = 0;
		mHeight = 0;
	}

	/// <summary>
	/// 割り当てられたインデックス
	/// </summary>
	/// <returns></returns>
	uint32_t Texture::GetDescriptorIndex() const
	{
		return static_cast<uint32_t>(mSrvHeap.GetIndex());
	}

	/// <summary>
	/// 割り当てられたGpuハンドルの取得
	/// </summary>
	/// <returns></returns>
	D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetGpuHandle()const
	{
		return mSrvHeap.GetGpuHandle();
	}

	/// <summary>
	/// テクスチャの幅
	/// </summary>
	/// <returns></returns>
	float Texture::GetWidth()const
	{
		return mWidth;
	}

	/// <summary>
	/// テクスチャの高さ
	/// </summary>
	/// <returns></returns>
	float Texture::GetHeight()const
	{
		return mHeight;
	}

	/// <summary>
	/// リソースの取得
	/// </summary>
	/// <returns></returns>
	ID3D12Resource* Texture::GetResource()const
	{
		return mResource.Get();
	}

}