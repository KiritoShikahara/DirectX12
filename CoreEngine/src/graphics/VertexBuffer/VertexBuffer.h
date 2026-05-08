#pragma once
#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
	class ENGINE_API VertexBuffer
	{
	public:
		VertexBuffer();
		virtual ~VertexBuffer();

		// GPUリソース持っているので、コピーは禁止（ムーブは必要に応じて実装検討）
		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer& operator=(const VertexBuffer&) = delete;

		/// <summary>
		/// 頂点バッファの作成
		/// </summary>
		/// <param name="size">バッファのサイズ</param>
		/// <param name="stride">1頂点のデータサイズ</param>
		/// <returns>true:成功</returns>
		bool Create(const size_t Size, const size_t Stride);

		/// <summary>
		/// バッファの解放
		/// </summary>
		void Release();


		/// <summary>
		/// CPU上のデータをバッファへ転送する
		/// </summary>
		/// <param name="SrcData">Map/Unmapを介さない永続マッピングポインタへのmemcpyにより、最小限のコストで更新します</param>
		/// <param name="Size">転送するサイズ</param>
		/// <param name="Offset">バッファ先頭からの書き込みオフセット</param>
		void Update(const void* SrcData, size_t Size, size_t Offset = 0);

		/// <summary>
		/// 頂点バッファビューをコマンドリストにセット
		/// </summary>
		/// <param name="CmdList">描画命令を記録するコマンドリスト</param>
		/// <param name="Slot">セットする入力スロットのインデックス</param>
		void Set(ID3D12GraphicsCommandList* CmdList, uint32_t Slot = 0) const;

		/// <summary>
		/// バッファの一部範囲や、異なるストライドとしてビューをセット
		/// </summary>
		/// <param name="CmdList"></param>
		/// <param name="Offset">ビューの開始地点オフセット</param>
		/// <param name="Size">ビューの対象サイズ</param>
		/// <param name="Stride">1頂点あたりのサイズ</param>
		/// <param name="Slot">入力スロット</param>
		void Set(ID3D12GraphicsCommandList* CmdList, size_t Offset, size_t Size, size_t Stride, uint32_t Slot = 0) const;

		/// <summary>
		/// 確保済みバッファの全バイトサイズ取得
		/// </summary>
		/// <returns></returns>
		size_t GetSize() const;

		/// <summary>
		/// 1頂点あたりのバイトサイズを取得
		/// </summary>
		/// <returns></returns>
		size_t GetStride() const;

		const D3D12_VERTEX_BUFFER_VIEW& GetView() { return mBufferView; }

	private:
		/// <summary>
		/// キャッシュされたバッファビュー
		/// </summary>
		D3D12_VERTEX_BUFFER_VIEW mBufferView;

		/// <summary>
		/// リソース本体
		/// </summary>
		Resource mBufferResource;

		/// <summary>
		/// バッファの全容量
		/// </summary>
		size_t mBufferSize;

		/// <summary>
		/// 1頂点のサイズ
		/// </summary>
		size_t mStride;

		/// <summary>
		/// Mapされた書き込み用CPUアドレス
		/// </summary>
		void* mMapped;
	};
}


