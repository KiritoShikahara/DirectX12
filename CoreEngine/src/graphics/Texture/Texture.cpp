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
	bool Texture::Create(const std::filesystem::path& FilePath)
	{

		if (fs::exists(FilePath) == false || fs::is_regular_file(FilePath) == false)
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Texture: File not found or is not a regular file: {}", FilePath.string());
			return false;
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
			//DEBUG_LOG(sys::eLogLevel::Error, "Texture: Failed to load texture file: {}", FilePath.string());
			return false;
		}

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
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = metaData.format;
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
		mSrvHeap = GDescriptorHeap{};

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