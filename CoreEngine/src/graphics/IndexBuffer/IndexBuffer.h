#pragma once

#include <Utility/Export/Export.h>
#include <graphics/Dx12/Dx12Type.h>
#include <cstdint>

namespace graphics
{
	class IndexBuffer
	{
	public:
		IndexBuffer();
		virtual ~IndexBuffer();

		IndexBuffer(const IndexBuffer&) = delete;
		IndexBuffer& operator=(const IndexBuffer&) = delete;

		// 動的インデックスバッファの作成
		bool CreateDynamic(size_t Size, DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);

		// 静的インデックスバッファの作成
		bool CreateStatic(ID3D12GraphicsCommandList* CmdList, const void* InitData, size_t Size, DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);

		// 同期的な静的インデックスバッファの作成
		bool CreateStaticSync(const void* InitData, size_t Size,
			DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);

		// バッファの解放
		void Release();

		// アップロード用の一時バッファを解放
		void ReleaseUploadBuffer();

		// CPU 側のデータをバッファへ転送
		void Update(const void* SrcData, size_t Size, size_t Offset = 0);

		// インデックスバッファビューをコマンドリストにセット
		void Set(ID3D12GraphicsCommandList* CmdList) const;

		// 確保済みバッファの全バイトサイズ取得
		size_t GetSize() const;

		// インデックス数を取得
		uint32_t GetCount() const;

		// インデックスのフォーマット取得
		DXGI_FORMAT GetFormat() const;

		const D3D12_INDEX_BUFFER_VIEW& GetView() const { return mBufferView; }

	private:
		// キャッシュされたバッファビュー
		D3D12_INDEX_BUFFER_VIEW mBufferView;

		// D3D12MA アロケーション
		MAAllocation mBufferAllocation;

		// リソース本体
		Resource mBufferResource;

		// 静的バッファ転送用の一時リソース
		Resource mUploadResource;

		// バッファの全容量
		size_t mBufferSize;

		// インデックスのフォーマット
		DXGI_FORMAT mFormat;

		// Map された CPU アドレス
		void* mMapped;

		// 動的 / 静的を判別するフラグ
		bool mIsDynamic = true;

	};
}