#pragma once
#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>

#include<array>

namespace graphics
{
	/// <summary>
	/// 頂点バッファ。
	///
	/// Dynamic  : UPLOAD ヒープに FRAME_COUNT 個のリソースを確保し、永続マップする。
	///            Update() / Set() は RenderContext::GetFrameIndex() が示すフレームの
	///            リソースに対して行われるため、GPU が読み取り中のバッファを
	///            CPU が上書きすることがない。
	/// Static   : DEFAULT ヒープに 1 個だけ確保する。作成後は書き換え不可。
	/// </summary>
	class ENGINE_API VertexBuffer
	{
		/// <summary>
		/// 現在のフレームインデックスを取得するヘルパー。
		/// RenderContext から取得する。
		/// </summary>
		/// <returns>[0, FRAME_COUNT) のインデックス</returns>
		uint32_t GetCurrentIndex() const;

	public:
		VertexBuffer();
		virtual ~VertexBuffer();

		// GPUリソースを持っているので、コピーは禁止（ムーブは必要に応じて実装する）
		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer& operator=(const VertexBuffer&) = delete;

		/// <summary>
		/// 動的頂点バッファの作成。
		/// FRAME_COUNT 個の UPLOAD リソースを確保して永続マップする。
		/// 毎フレーム Update() で内容を書き換える用途に使う。
		/// </summary>
		/// <param name="Size">バッファ 1 個あたりのサイズ（バイト）</param>
		/// <param name="Stride">1 頂点のデータサイズ（バイト）</param>
		/// <returns>true:成功</returns>
		bool CreateDynamic(const size_t Size, const size_t Stride);

		/// <summary>
		/// 静的な頂点バッファの作成（コマンドリスト経由）。
		/// CmdList に Copy コマンドを積むため、実行・完了待ちは呼び出し側の責任。
		/// 転送完了後は ReleaseUploadBuffer() で一時リソースを解放すること。
		/// </summary>
		/// <returns>true:成功</returns>
		bool CreateStatic(ID3D12GraphicsCommandList* CmdList, const void* InitData, const size_t Size, const size_t Stride);

		/// <summary>
		/// 同期的な静的頂点バッファの作成。
		/// DX12Device の専用アップロードキューを使うため cmdList 不要。
		/// DEFAULT ヒープに D3D12MA で確保し内部で完了待ちする。
		/// スレッドセーフ（内部の UploadBufferData が mutex で保護される）。
		/// </summary>
		bool CreateStaticSync(const void* InitData, size_t Size, size_t Stride);

		/// <summary>
		/// バッファの解放
		/// </summary>
		void Release();

		/// <summary>
		/// アップロード用の一時バッファを解放する（CreateStatic 使用時のみ意味を持つ）
		/// </summary>
		void ReleaseUploadBuffer();

		/// <summary>
		/// CPU 上のデータを現在フレームのバッファへ転送する。
		/// Map/Unmap は行わない（永続マッピングポインタへの memcpy）。
		/// Dynamic で作成したバッファにのみ有効。
		///
		/// 注意: 書き込まれるのは現在フレームのリソースのみ。
		///       内容が不変なバッファは Update() ではなく CreateStaticSync() を使うこと。
		/// </summary>
		/// <param name="SrcData">転送元データ</param>
		/// <param name="Size">転送するサイズ（バイト）</param>
		/// <param name="Offset">バッファ先頭からの書き込みオフセット（バイト）</param>
		void Update(const void* SrcData, size_t Size, size_t Offset = 0);

		/// <summary>
		/// 全フレーム分のバッファへ同一データを書き込む。
		/// Dynamic バッファを「初期化時に一度だけ書いて以降変えない」用途で使う場合に呼ぶ。
		/// （本来そのような用途は CreateStaticSync() が正しい）
		/// </summary>
		/// <param name="SrcData">転送元データ</param>
		/// <param name="Size">転送するサイズ（バイト）</param>
		/// <param name="Offset">バッファ先頭からの書き込みオフセット（バイト）</param>
		void UpdateAll(const void* SrcData, size_t Size, size_t Offset = 0);

		/// <summary>
		/// 頂点バッファビューをコマンドリストにセットする。
		/// Dynamic の場合は現在フレームのビューが使われる。
		/// </summary>
		/// <param name="CmdList">描画命令を記録するコマンドリスト</param>
		/// <param name="Slot">セットする入力スロットのインデックス</param>
		void Set(ID3D12GraphicsCommandList* CmdList, uint32_t Slot = 0) const;

		/// <summary>
		/// バッファの一部範囲を、異なるストライドとしてビューをセットする
		/// </summary>
		/// <param name="CmdList">描画命令を記録するコマンドリスト</param>
		/// <param name="Offset">ビューの開始地点オフセット</param>
		/// <param name="Size">ビューの対象サイズ</param>
		/// <param name="Stride">1 頂点あたりのサイズ</param>
		/// <param name="Slot">入力スロット</param>
		void Set(ID3D12GraphicsCommandList* CmdList, size_t Offset, size_t Size, size_t Stride, uint32_t Slot = 0) const;

		/// <summary>
		/// 確保済みバッファの全バイトサイズ取得（1 フレーム分）
		/// </summary>
		size_t GetSize() const;

		/// <summary>
		/// 1 頂点あたりのバイトサイズを取得
		/// </summary>
		size_t GetStride() const;

		/// <summary>
		/// 現在フレームの頂点バッファビューを取得
		/// </summary>
		const D3D12_VERTEX_BUFFER_VIEW& GetView() const;

		/// <summary>
		/// 有効なバッファかどうか
		/// </summary>
		bool IsValid() const;

	private:
		/// <summary>
		/// Dynamic 用のフレームごとのリソース
		/// </summary>
		struct DynamicFrame
		{
			/// <summary>UPLOAD ヒープのリソース本体</summary>
			Resource                 BufferResource = nullptr;
			/// <summary>D3D12MA アロケーション</summary>
			MAAllocation             Allocation = nullptr;
			/// <summary>永続マップされた書き込み用 CPU アドレス</summary>
			void* Mapped = nullptr;
			/// <summary>キャッシュされたバッファビュー</summary>
			D3D12_VERTEX_BUFFER_VIEW View = {};
		};

		/// <summary>
		/// Dynamic 用リソース（FRAME_COUNT 個のリングバッファ）
		/// </summary>
		std::array<DynamicFrame, FRAME_COUNT> mDynamicFrames;

		// ---- Static 用 ----

		/// <summary>キャッシュされたバッファビュー（Static のみ使用）</summary>
		D3D12_VERTEX_BUFFER_VIEW mBufferView;

		/// <summary>D3D12MA アロケーション（CreateStaticSync 使用時のみ有効）</summary>
		MAAllocation mBufferAllocation;

		/// <summary>リソース本体（Static のみ使用）</summary>
		Resource mBufferResource;

		/// <summary>静的バッファ転送用の一時リソース（CreateStatic 使用時のみ有効）</summary>
		Resource mUploadResource;

		// ---- 共通 ----

		/// <summary>バッファの全容量（1 フレーム分）</summary>
		size_t mBufferSize;

		/// <summary>1 頂点のサイズ</summary>
		size_t mStride;

		/// <summary>動的か静的かを判別するフラグ</summary>
		bool mIsDynamic = true;
	};
}