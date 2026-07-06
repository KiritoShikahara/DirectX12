#pragma once

#include  <utility/Singleton/Singleton.hpp>
#include <Utility/Export/Export.h>
#include<vector>
#include<mutex>

#include "Dx12Type.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
namespace graphics
{
	/// <summary>
	/// Dx12デバイス管理
	/// </summary>
	class ENGINE_API DX12Device : public utility::Singleton<DX12Device>
	{
		DX12Device();
		SINGLETON_CLASS_CUSTOM_CTOR(DX12Device);
	public:
		SINGLETON_ACCESSOR(DX12Device);

		/// <summary>
		/// 初期化
		/// </summary>
		/// <returns>true:成功　</returns>
		bool Initialize();

		/// <summary>
		/// 終了処理
		/// </summary>
		/// <returns></returns>
		bool Finalize();

		/// <summary>
		/// Dx12デバイスの取得
		/// </summary>
		ID3D12Device* GetDevice();

		/// <summary>
		/// DXGIファクトリーの取得
		/// </summary>
		IDXGIFactory7* GetFactory();

		/// <summary>
		/// D3D12MAアロケーターの取得
		/// </summary>
		D3D12MA::Allocator* GetMAAllocator();

		/// <summary>
		/// GPUにテクスチャリソースを転送する。
		/// 専用のアップロードキューで実行するため描画ループに依存しない。
		/// </summary>
		/// <param name="pResource">転送先リソース</param>
		/// <param name="subresources">転送するサブリソースのデータ</param>
		/// <returns>true:成功</returns>
		bool UploadTextureData(ID3D12Resource* pResource,
			const std::vector<D3D12_SUBRESOURCE_DATA>& subresources);

		/// <summary>
		/// GPU にバッファデータを転送する。
		/// UploadTextureData と同じく専用アップロードキューで同期的に完結する。
		/// スレッドセーフ (内部で mutex によって排他制御される)。
		/// cmdList は不要。描画ループに依存しない。
		/// </summary>
		/// <param name="pResource">転送先リソース (DEFAULT heap, COPY_DEST 状態で作成済み)</param>
		/// <param name="data">転送するデータポインタ (nullptr 禁止)</param>
		/// <param name="size">転送バイト数 (0 禁止)</param>
		/// <param name="targetState">転送完了後のリソース状態</param>
		bool UploadBufferData(
			ID3D12Resource* pResource,
			const void* data,
			size_t                size,
			D3D12_RESOURCE_STATES targetState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	private:
		/// <summary>
		/// デバッグレイヤーの有効化（デバッグビルドのみ）
		/// </summary>
		void DebugLayerOn();

		/// <summary>
		/// DXGIファクトリーの初期化
		/// </summary>
		bool InitializeFactory();

		/// <summary>
		/// デバイスとD3D12MAアロケーターの初期化
		/// </summary>
		bool InitializeDevice();

		/// <summary>
		/// アップロード専用コンテキストの初期化
		/// （コマンドキュー・アロケーター・コマンドリスト・フェンス）
		/// </summary>
		bool InitializeUploadContext();

	private:
		/// <summary>GPUとの通信窓口</summary>
		Device          mDevice;
		/// <summary>スワップチェインやアダプタの作成に使う</summary>
		Factory         mFactory;
		/// <summary>D3D12MAのメモリアロケーター</summary>
		MAAllocator     mMAAllocator;
		/// <summary>リソース漏れ検知（デバッグビルドのみ有効）</summary>
		DebugDevice     mDebugDevice;

		// ---- アップロード専用コンテキスト ----
		/// <summary>アップロード専用コマンドキュー（描画キューと分離）</summary>
		CmdQueue        mUploadCmdQueue;
		/// <summary>アップロード専用コマンドアロケーター</summary>
		CmdAlloc        mUploadAllocator;
		/// <summary>アップロード専用コマンドリスト</summary>
		CmdList         mUploadCmdList;
		/// <summary>アップロード完了同期用フェンス</summary>
		Fence           mUploadFence;
		/// <summary>アップロード用フェンスカウンター</summary>
		UINT64          mUploadFenceValue = 0;
		/// <summary>アップロード完了待ちイベントハンドル</summary>
		HANDLE          mUploadEvent = nullptr;

		/// <summary>
		/// アップロードコンテキスト用の排他制御
		///  UploadTextureData / UploadBufferData を複数スレッドから同時に呼んだ場合に
        /// mUploadAllocator / mUploadCmdList への同時アクセスを防ぐ
		/// </summary>
		std::mutex mUploadMutex;

		bool mDebugLayerEnabled = false;
	};
}


