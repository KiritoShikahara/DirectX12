#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>

#include<DirectXTex.h>
#include<filesystem>

namespace graphics
{
	class ENGINE_API Texture
	{
	public:
		Texture();
		virtual ~Texture();

		/// <summary>
		/// 生成
		/// </summary>
		/// <param name="FilePath"></param>
		/// <param name="isSRGB"></param>
		/// <returns></returns>
		bool Create(const std::filesystem::path& FilePath, bool isSRGB = false);
		
		/// <summary>
		/// CPU側の画像データ
		/// </summary>
		struct ImageData
		{
			bool Success = false;
			DirectX::TexMetadata MetaData = {};
			DirectX::ScratchImage ScratchImage;
		};

		/// <summary>
		/// CPU側の画像データをロードする
		/// </summary>
		static ImageData LoadImageData(const std::filesystem::path& FilePath, bool isSRGB);

		/// <summary>
		/// GPU側のテクスチャを生成する
		/// </summary>
		bool CreateFromImageData(const std::filesystem::path& FilePath, bool isSRGB, const ImageData& imageData);

		/// <summary>
		/// リソース解放
		/// </summary>
		void Release();

		/// <summary>
		/// ディスクリプタのインデックスを取得
		/// </summary>
		/// <returns></returns>
		uint32_t GetDescriptorIndex() const;

		/// <summary>
		/// GPUハンドルの取得
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;

		/// <summary>
		/// 横サイズ
		/// </summary>
		float GetWidth()const;

		/// <summary>
		/// 縦サイズ
		/// </summary>
		float GetHeight()const;

		/// <summary>
		/// リソースの取得
		/// </summary>
		/// <returns></returns>
		ID3D12Resource* GetResource() const;

		bool IsValid()   const { return mSrvHeap.IsValid(); }
	private:
		/// <summary>
		/// リソース
		/// </summary>
		Resource mResource;

		/// <summary>
		/// MAリソース
		/// </summary>
		MAAllocation mAllocation;

		/// <summary>
		/// GDHのスロット
		/// </summary>
		graphics::GDescriptorHeap mSrvHeap;

		/// <summary>
		/// 横サイズ
		/// </summary>
		float mWidth;

		/// <summary>
		/// 縦サイズ
		/// </summary>
		float mHeight;
	};
}