#pragma once

#include  <utility/Singleton/Singleton.hpp>
#include <Utility/Export/Export.h>
#include<vector>

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
	public:
		static constexpr int FRAME_COUNT = 3;


	};
}


