#include "pch.h"
#include "Dx12Device.h"
#include<initguid.h>
#include<d3dx12.h>

namespace graphics
{
	DX12Device::DX12Device()
		:mDevice(nullptr)
		, mFactory(nullptr)
		, mMAAllocator(nullptr)
		, mDebugDevice(nullptr)
		, mUploadCmdQueue(nullptr)
		, mUploadAllocator(nullptr)
		, mUploadCmdList(nullptr)
		, mUploadFence(nullptr)
		, mUploadFenceValue(0)
		, mUploadEvent(nullptr)
	{
	}

	bool DX12Device::Initialize()
	{
#if defined(_DEBUG) || ECSE_DEV_TOOL_ENABLED 
		//	デバック時だけリソース検知などを有効に
		DebugLayerOn();
#endif

		// COMの初期化
		HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(hr))
		{
			return false;
		}

		// ファクトリーの初期化
		if (InitializeFactory() == false)
		{			
			// TODO:ログ出力
			return false;
		}

		// デバイスとD3D12MAアロケーターの初期化
		if (InitializeDevice() == false)
		{
			// TODO:ログ出力
			return false;
		}

		// アップロード専用コンテキストの初期化
		if (InitializeUploadContext() == false)
		{
			// TODO:ログ出力
			return false;
		}


		// TODO:ログ出力
		return true;

	}

	bool DX12Device::Finalize()
	{
		// アップロードの完了を待ってからリソースを解放する
		if (mUploadCmdQueue && mUploadFence)
		{
			mUploadFenceValue++;
			mUploadCmdQueue->Signal(mUploadFence.Get(), mUploadFenceValue);
			if (mUploadFence->GetCompletedValue() < mUploadFenceValue)
			{
				mUploadFence->SetEventOnCompletion(mUploadFenceValue, mUploadEvent);
				WaitForSingleObject(mUploadEvent, INFINITE);
			}
		}

		if (mUploadEvent != nullptr)
		{
			CloseHandle(mUploadEvent);
			mUploadEvent = nullptr;
		}

		mUploadFence.Reset();
		mUploadCmdList.Reset();
		mUploadAllocator.Reset();
		mUploadCmdQueue.Reset();

		mMAAllocator.Reset();
		mFactory.Reset();

#if defined(_DEBUG) || ECSE_DEV_TOOL_ENABLED
		if (mDebugDevice != nullptr)
		{
			mDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
			mDebugDevice.Reset();
		}
#endif
		mDevice.Reset();


		return true;
	}

	bool DX12Device::UploadTextureData(ID3D12Resource* pResource, const std::vector<D3D12_SUBRESOURCE_DATA>& subresources)
	{
		if (pResource == nullptr || subresources.empty()) return false;

		// アロケーターとコマンドリストをリセット
		mUploadAllocator->Reset();
		mUploadCmdList->Reset(mUploadAllocator.Get(), nullptr);

		const UINT   numSub = static_cast<UINT>(subresources.size());
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(pResource, 0, numSub);

		// D3D12MAを使って中間バッファ（ステージングバッファ）を確保
		D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
		uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		Resource     uploadRes;
		MAAllocation uploadAlloc;
		auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		HRESULT hr = mMAAllocator->CreateResource(
			&uploadAllocDesc,
			&uploadBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			&uploadAlloc,
			IID_PPV_ARGS(&uploadRes));

		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		// 中間バッファ経由でVRAMリソースへコピー
		UpdateSubresources(mUploadCmdList.Get(), pResource, uploadRes.Get(),
			0, 0, numSub, subresources.data());

		// 転送完了後、ピクセルシェーダーで読める状態へ遷移
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pResource,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mUploadCmdList->ResourceBarrier(1, &barrier);

		// コマンド送信と完了待ち（専用キューで実行するため描画ループに依存しない）
		mUploadCmdList->Close();
		ID3D12CommandList* ppCommandLists[] = { mUploadCmdList.Get() };
		mUploadCmdQueue->ExecuteCommandLists(1, ppCommandLists);

		mUploadFenceValue++;
		mUploadCmdQueue->Signal(mUploadFence.Get(), mUploadFenceValue);

		if (mUploadFence->GetCompletedValue() < mUploadFenceValue)
		{
			mUploadFence->SetEventOnCompletion(mUploadFenceValue, mUploadEvent);
			WaitForSingleObject(mUploadEvent, INFINITE);
		}

		return true;
	}

	/// <summary>
	/// GPU にバッファデータを転送する。
	/// UploadTextureData と同じく専用アップロードキューで同期的に完結する。
	/// スレッドセーフ (内部で mutex によって排他制御される)。
	/// cmdList は不要。描画ループに依存しない。
	/// </summary>
	bool DX12Device::UploadBufferData(ID3D12Resource* pResource, const void* data, size_t size, D3D12_RESOURCE_STATES targetState)
	{
		if (!pResource || !data || size == 0)
		{
			// TODO: ログ出力
			DEBUG_LOG(sys::eLogLevel::Error, "Fail UploadBufferData.");
			return false;
		}

		// UploadTextureData と同じアップロードコンテキストを排他的に使用
		std::lock_guard<std::mutex> lock(mUploadMutex);

		mUploadAllocator->Reset();
		mUploadCmdList->Reset(mUploadAllocator.Get(), nullptr);

		// ステージングバリア
		D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
		uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		Resource     uploadRes;
		MAAllocation uploadAlloc;
		auto         bufDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

		HRESULT hr = mMAAllocator->CreateResource(
			&uploadAllocDesc, &bufDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, &uploadAlloc, IID_PPV_ARGS(&uploadRes));
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Failed MAAlloc CreateResource.");
			return false;
		}

		// CPU->ステージング
		void* mapped = nullptr;
		uploadRes->Map(0, nullptr, &mapped);
		std::memcpy(mapped, data, size);
		uploadRes->Unmap(0, nullptr);
		
		// ステージング->GPU
		mUploadCmdList->CopyBufferRegion(pResource, 0, uploadRes.Get(), 0, size);

		// 転送後の状態遷移
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pResource,
			D3D12_RESOURCE_STATE_COPY_DEST,
			targetState);
		mUploadCmdList->ResourceBarrier(1, &barrier);

		// コマンド送信 + 同期待ち
		mUploadCmdList->Close();
		ID3D12CommandList* ppCmdLists[] = { mUploadCmdList.Get() };
		mUploadCmdQueue->ExecuteCommandLists(1, ppCmdLists);

		mUploadFenceValue++;
		mUploadCmdQueue->Signal(mUploadFence.Get(), mUploadFenceValue);
		if (mUploadFence->GetCompletedValue() < mUploadFenceValue)
		{
			mUploadFence->SetEventOnCompletion(mUploadFenceValue, mUploadEvent);
			WaitForSingleObject(mUploadEvent, INFINITE);
		}

		return true;

	}

	/// <summary>
	/// デバッグレイヤーの有効化（デバッグビルドのみ）
	/// </summary>
	void DX12Device::DebugLayerOn()
	{
		Debug5 debugLayer = nullptr;

		HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		if (SUCCEEDED(hr))
		{
			debugLayer->EnableDebugLayer();
			debugLayer->SetEnableAutoName(TRUE);
			mDebugLayerEnabled = true;
		}
		else
		{
			ComPtr<ID3D12Debug> debugBasic;
			hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugBasic));
			if (SUCCEEDED(hr))
			{
				debugBasic->EnableDebugLayer();
				mDebugLayerEnabled = true;
			}
			else
			{
				// グラフィックスツール未インストールなどで失敗。デバッグ機能なしで続行。
				DEBUG_LOG(sys::eLogLevel::Warning, "DebugLayer unavailable. Continuing without it.");
			}
		}
	}

	/// <summary>
	/// DXGIファクトリーの初期化
	/// </summary>
	bool DX12Device::InitializeFactory()
	{
		UINT factoryFlags = 0;

		// デバッグレイヤーが実際に有効化できた場合のみDXGIデバッグも要求する
		if (mDebugLayerEnabled)
		{
			factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}

		HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mFactory));
		if (FAILED(hr) && (factoryFlags & DXGI_CREATE_FACTORY_DEBUG))
		{
			// DXGIDebug.dllが無い等で失敗した場合、フラグ無しでリトライ
			DEBUG_LOG(sys::eLogLevel::Warning, "DXGIDebug unavailable. Retrying without debug flag.");
			factoryFlags &= ~DXGI_CREATE_FACTORY_DEBUG;
			hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mFactory));
		}

		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		return true;
	}

	/// <summary>
	/// デバイスとD3D12MAアロケーターの初期化
	/// </summary>
	bool DX12Device::InitializeDevice()
	{
		// 処理能力が高い順にGPUをリストアップし、最初に作れたものを使う
		Adapter adapter;
		for (UINT i = 0;
			mFactory->EnumAdapterByGpuPreference(
				i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
			i++)
		{
			DXGI_ADAPTER_DESC3 desc;
			adapter->GetDesc3(&desc);

			// ソフトウェアレンダラー（Microsoft Basic Render Driver等）は除外
			if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;

			// D3D_FEATURE_LEVEL_12_1 以上を要求
			HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice));
			if (SUCCEEDED(hr))
			{
				// D3D12MAアロケーターの作成
				D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
				allocatorDesc.pDevice = mDevice.Get();
				allocatorDesc.pAdapter = adapter.Get();

				hr = D3D12MA::CreateAllocator(&allocatorDesc, &mMAAllocator);
				if (FAILED(hr))
				{
					// TODO:ログ出力
					return false;
				}
				break;
			}
		}

		if (mDevice == nullptr)
		{
			// TODO:ログ出力
			return false;
		}

#if defined(_DEBUG) || ECSE_DEV_TOOL_ENABLED
		if (FAILED(mDevice.As(&mDebugDevice)))
		{
			// TODO:ログ出力
		}
#endif
		return true;
	}

	/// <summary>
	/// アップロード専用コンテキストの初期化
	/// （コマンドキュー・アロケーター・コマンドリスト・フェンス）
	/// </summary>
	bool DX12Device::InitializeUploadContext()
	{
		HRESULT hr = S_OK;

		// 描画キューとは独立した専用のダイレクトキューを作成
		// （COPYキューはGetRequiredIntermediateSizeの互換性上、DIRECTを使用）
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;

		hr = mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mUploadCmdQueue));
		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&mUploadAllocator));
		if (FAILED(hr)) return false;

		hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			mUploadAllocator.Get(), nullptr,
			IID_PPV_ARGS(&mUploadCmdList));
		if (FAILED(hr)) return false;

		// 最初は記録しない状態にしておく
		mUploadCmdList->Close();

		hr = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mUploadFence));
		if (FAILED(hr)) return false;

		mUploadFenceValue = 0;

		mUploadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mUploadEvent == nullptr) return false;

		// TODO:ログ出力
		return true;

	}

	/// <summary>
	/// Dx12デバイスの取得
	/// </summary>
	ID3D12Device* graphics::DX12Device::GetDevice()
	{
		return mDevice.Get();
	}

	/// <summary>
	/// DXGIファクトリーの取得
	/// </summary>
	IDXGIFactory7* DX12Device::GetFactory()
	{
		return mFactory.Get();
	}

	/// <summary>
	/// D3D12MAアロケーターの取得
	/// </summary>
	D3D12MA::Allocator* DX12Device::GetMAAllocator()
	{
		return mMAAllocator.Get();
	}
}