#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>

#include<filesystem>

namespace graphics
{
	class ENGINE_API Texture
	{
	public:
		Texture();
		virtual ~Texture();

		/// <summary>
		/// リソースの作成
		/// </summary>
		/// <param name="FilePath">テクスチャファイルのパス</param>
		/// <returns>作成に成功した場合はtrue、失敗した場合はfalse</returns>
		bool Create(const std::filesystem::path& FilePath);

		/// <summary>
		/// リソースの解放
		/// </summary>
		void Release();

		/// <summary>
		/// 割り当てられたインデックス
		/// </summary>
		/// <returns></returns>
		uint32_t GetDescriptorIndex() const;

		/// <summary>
		/// 割り当てられたGpuハンドルの取得
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;

		/// <summary>
		/// テクスチャの幅
		/// </summary>
		/// <returns></returns>
		float GetWidth()const;
		/// <summary>
		/// テクスチャの高さ
		/// </summary>
		/// <returns></returns>
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
		/// メモリ割り当て情報
		/// </summary>
		MAAllocation mAllocation;

		/// <summary>
		/// SRV用のディスクリプタスロット
		/// </summary>
		graphics::GDescriptorHeap mSrvHeap;

		/// <summary>
		/// テクスチャ横幅
		/// </summary>
		float mWidth;
		/// <summary>
		/// テクスチャ縦幅
		/// </summary>
		float mHeight;
	};
}