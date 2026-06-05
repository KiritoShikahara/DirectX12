#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>
#include<cstdint>

namespace graphics
{
	class IndexBuffer
	{
	public:
		IndexBuffer();
		virtual ~IndexBuffer();

		// GPU リソースを持つため、コピーは禁止（ムーブは必要に応じて実装）
		IndexBuffer(const IndexBuffer&) = delete;
		IndexBuffer& operator=(const IndexBuffer&) = delete;


		/// <summary>
		/// 動的インデックスバッファの作成（CPU から毎フレーム更新可能）
		/// </summary>
		/// <param name="Size">バッファのバイトサイズ</param>
		/// <param name="Format">インデックスのフォーマット（DXGI_FORMAT_R16_UINT / DXGI_FORMAT_R32_UINT）</param>
		/// <returns>true: 成功</returns>
		bool CreateDynamic(size_t Size, DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);

		/// <summary>
		/// 静的インデックスバッファの作成（GPU 専用メモリへ転送）
		/// </summary>
		/// <param name="CmdList">アップロード命令を記録するコマンドリスト</param>
		/// <param name="InitData">初期データへのポインタ</param>
		/// <param name="Size">バッファのバイトサイズ</param>
		/// <param name="Format">インデックスのフォーマット（DXGI_FORMAT_R16_UINT / DXGI_FORMAT_R32_UINT）</param>
		/// <returns>true: 成功</returns>
		bool CreateStatic(ID3D12GraphicsCommandList* CmdList, const void* InitData, size_t Size, DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);

		/// <summary>
		/// 同期的な静的インデックスバッファの作成。
		/// DX12Device の専用アップロードキューを使うため cmdList 不要。
		/// DEFAULT ヒープは D3D12MA で確保し断片化を抑制。
		/// スレッドセーフ (内部の UploadBufferData が mutex で保護)。
		/// </summary>
		bool CreateStaticSync(const void* InitData, size_t Size,
                         DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);

		/// <summary>
		/// バッファの解放
		/// </summary>
		void Release();

		/// <summary>
		/// アップロード用の一時バッファを解放（GPU転送完了後に呼ぶ）
		/// </summary>
		void ReleaseUploadBuffer();

		/// <summary>
		/// CPU 側のデータをバッファへ転送する（動的バッファ専用）
		/// Map/Unmap を行わない永続マッピングポインタへの memcpy により、最小コストで更新する
		/// </summary>
		/// <param name="SrcData">転送元データへのポインタ</param>
		/// <param name="Size">転送するバイトサイズ</param>
		/// <param name="Offset">バッファ先頭からの書き込みオフセット</param>
		void Update(const void* SrcData, size_t Size, size_t Offset = 0);

		/// <summary>
		/// インデックスバッファビューをコマンドリストにセット
		/// </summary>
		/// <param name="CmdList">描画命令を記録するコマンドリスト</param>
		void Set(ID3D12GraphicsCommandList* CmdList) const;

		/// <summary>
		/// 確保済みバッファの全バイトサイズ取得
		/// </summary>
		size_t GetSize() const;

		/// <summary>
		/// インデックス数を取得（Size / インデックス1個のバイトサイズ）
		/// </summary>
		uint32_t GetCount() const;

		/// <summary>
		/// インデックスのフォーマット取得
		/// </summary>
		DXGI_FORMAT GetFormat() const;

		const D3D12_INDEX_BUFFER_VIEW& GetView() const { return mBufferView; }

	private:
		/// <summary>
		/// キャッシュされたバッファビュー
		/// </summary>
		D3D12_INDEX_BUFFER_VIEW mBufferView;

		/// <summary>
		/// D3D12MA アロケーション (CreateStaticSync 使用時のみ有効)
		/// </summary>
		MAAllocation mBufferAllocation;

		/// <summary>
		/// リソース本体（Default ヒープ or Upload ヒープ）
		/// </summary>
		Resource mBufferResource;

		/// <summary>
		/// 静的バッファ転送用の一時リソース（Upload ヒープ）
		/// </summary>
		Resource mUploadResource;

		/// <summary>
		/// バッファの全容量（バイト）
		/// </summary>
		size_t mBufferSize;

		/// <summary>
		/// インデックスのフォーマット（R16_UINT / R32_UINT）
		/// </summary>
		DXGI_FORMAT mFormat;

		/// <summary>
		/// Map された CPU アドレス（動的バッファのみ有効）
		/// </summary>
		void* mMapped;

		/// <summary>
		/// 動的 / 静的を判別するフラグ
		/// </summary>
		bool mIsDynamic = true;

	};
}

