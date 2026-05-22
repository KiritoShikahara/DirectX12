#include "pch.h"
#include "Dx12Renderer.h"
#include"Dx12Device.h"

namespace graphics
{
	DX12Context::DX12Context()
		: mDeviceService(nullptr)
		, mSwapChain(nullptr)
		 , mCmdQueue(nullptr)
		 , mCmdList(nullptr)
		 , mDepthBuffer(nullptr)
		 , mRtvHeap(nullptr)
		 , mDsvHeap(nullptr)
		 , mFence(nullptr)
		, mWaitForGPUEventHandle(nullptr)
	{
		mClearColor = graphics::Color::Gray;
	}

	DX12Context::~DX12Context()
	{
	}

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="pDevice">初期化済みの DX12Device</param>
	/// <param name="WindowHandle">対象ウィンドウのハンドル</param>
	/// <param name="Width">スクリーン横幅</param>
	/// <param name="Height">スクリーン縦幅</param>
	/// <returns>true:成功</returns>
	bool DX12Context::Initialize(DX12Device* pDevice, HWND WindowHandle, UINT Width, UINT Height)
	{
		if (pDevice == nullptr) return false;
		mDeviceService = pDevice;

		if (InitializeCommandObjects() == false)
		{
			return false;
		}

		if (InitializeSwapChain(WindowHandle, Width, Height) == false)
		{
			return false;
		}

		if (InitializeBackBufferHeap() == false)
		{
			return false;
		}

		if (InitializeDepthHeap(Width, Height) == false)
		{
			return false;
		}

		if (InitializeFence() == false)
		{
			return false;
		}

		return true;
	}

	bool DX12Context::Finalize()
	{
		WaitForGPU();

		if (mWaitForGPUEventHandle != nullptr)
		{
			CloseHandle(mWaitForGPUEventHandle);
			mWaitForGPUEventHandle = nullptr;
		}

		for (auto& frame : mFrames)
		{
			frame.BackBuffer.Reset();
			frame.Allocator.Reset();
			frame.UploadPool.Reset();
		}

		mDepthBuffer.Reset();
		mRtvHeap.Reset();
		mDsvHeap.Reset();

		mCmdList.Reset();
		mSwapChain.Reset();   // CommandQueueより先に解放
		mCmdQueue.Reset();

		mFence.Reset();

		return true;
	}

	/// <summary>
	/// フレーム描画の開始
	/// （バックバッファのクリア・レンダーターゲット設定）
	/// </summary>
	void DX12Context::BeginRendering()
	{
		// 次に描画するバックバッファのインデックスを取得
		mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

		// そのフレームのGPU処理が終わっていなければ待機（ストール防止）
		if (mFence->GetCompletedValue() < mFrames[mFrameIndex].FenceValue)
		{
			mFence->SetEventOnCompletion(mFrames[mFrameIndex].FenceValue, mWaitForGPUEventHandle);
			WaitForSingleObject(mWaitForGPUEventHandle, INFINITE);
		}

		// コマンド記録の開始
		mFrames[mFrameIndex].Allocator->Reset();
		mCmdList->Reset(mFrames[mFrameIndex].Allocator.Get(), nullptr);

		// バックバッファをPRESENTからRENDER_TARGETへ遷移
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = mFrames[mFrameIndex].BackBuffer.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		mCmdList->ResourceBarrier(1, &barrier);

		// レンダーターゲットの設定
		const UINT rtvIncSize = mDeviceService->GetDevice()
			->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += mFrameIndex * rtvIncSize;

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
		mCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// レンダーターゲットと深度バッファをクリア
		mCmdList->ClearRenderTargetView(rtvHandle, mClearColor.GetRawPointer(), 0, nullptr);
		mCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	}

	/// <summary>
	/// 画面のフリップ（コマンド送信・Present）
	/// </summary>
	void DX12Context::Flip()
	{
		// バックバッファをRENDER_TARGETからPRESENTへ遷移
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = mFrames[mFrameIndex].BackBuffer.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		mCmdList->ResourceBarrier(1, &barrier);

		// コマンドリストを確定してGPUへ送信
		mCmdList->Close();
		ID3D12CommandList* ppCommandLists[] = { mCmdList.Get() };
		mCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// 画面の切り替え
		mSwapChain->Present(1, 0);

		// このフレームの完了フェンス値を記録
		mNextFenceValue++;
		mFrames[mFrameIndex].FenceValue = mNextFenceValue;
		mCmdQueue->Signal(mFence.Get(), mNextFenceValue);
	}

	/// <summary>
	/// 全GPUコマンドの完了を待機する
	/// </summary>
	void DX12Context::WaitForGPU()
	{
		// 現在の値でSignalして完了まで待機
		mNextFenceValue++;
		if (FAILED(mCmdQueue->Signal(mFence.Get(), mNextFenceValue)))
		{
			return;
		} 
		if (mFence->GetCompletedValue() < mNextFenceValue)
		{
			mFence->SetEventOnCompletion(mNextFenceValue, mWaitForGPUEventHandle);
			WaitForSingleObject(mWaitForGPUEventHandle, INFINITE);
		}
	}

	/// <summary>
	/// ビューポートとシザー矩形の設定
	/// </summary>
	void DX12Context::SetViewPort(float Width, float Height, float x, float y)
	{
		D3D12_VIEWPORT viewport = { x, y, Width, Height, 0.0f, 1.0f };
		D3D12_RECT scissor = { (LONG)x, (LONG)y, (LONG)(x + Width), (LONG)(y + Height) };

		mCmdList->RSSetViewports(1, &viewport);
		mCmdList->RSSetScissorRects(1, &scissor);
	}

	/// <summary>
	/// 描画用コマンドリストの取得
	/// </summary>
	ID3D12GraphicsCommandList* DX12Context::GetCommandList()
	{
		return mCmdList.Get();
	}

	/// <summary>
	/// 現在フレームのコマンドアロケーターの取得
	/// </summary>
	ID3D12CommandAllocator* DX12Context::GetCommandAllocator()
	{
		return mFrames[mFrameIndex].Allocator.Get();
	}

	/// <summary>
	/// コマンドキューの取得
	/// </summary>
	ID3D12CommandQueue* DX12Context::GetCommandQueue()
	{
		return mCmdQueue.Get();
	}

	/// <summary>
	/// 現在フレームのD3D12MAアップロードプールの取得
	/// </summary>
	D3D12MA::Pool* DX12Context::GetMAUploadPool()
	{
		return mFrames[mFrameIndex].UploadPool.Get();
	}

	/// <summary>
	/// 現在フレームのインデックスの取得
	/// </summary>
	UINT DX12Context::GetCurrentFrameIndex()const
	{
		return mFrameIndex;
	}

	bool DX12Context::InitializeCommandObjects()
	{
		ID3D12Device* device = mDeviceService->GetDevice();
		HRESULT hr = S_OK;

		// コマンドキューの作成
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;

		hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue));
		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		// フレームごとのコマンドアロケーターとアップロードプールを作成
		D3D12MA::Allocator* maAllocator = mDeviceService->GetMAAllocator();
		for (int i = 0; i < FRAME_COUNT; i++)
		{
			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&mFrames[i].Allocator));
			if (FAILED(hr))
			{
				// TODO:ログ出力
				return false;
			}

			D3D12MA::POOL_DESC poolDesc = {};
			poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			poolDesc.Flags = D3D12MA::POOL_FLAG_ALGORITHM_LINEAR;

			hr = maAllocator->CreatePool(&poolDesc, &mFrames[i].UploadPool);
			if (FAILED(hr))
			{
				// TODO:ログ出力
				return false;
			}
		}

		// コマンドリストの作成（最初はClose状態）
		hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			mFrames[0].Allocator.Get(), nullptr,
			IID_PPV_ARGS(&mCmdList));
		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}
		mCmdList->Close();

		return true;
	}

	bool DX12Context::InitializeSwapChain(HWND WindowHandle, UINT Width, UINT Height)
	{
		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = Width;
		scDesc.Height = Height;
		scDesc.Format = mFormat;
		scDesc.Stereo = FALSE;
		scDesc.SampleDesc.Count = 1;   // マルチサンプルOFF
		scDesc.SampleDesc.Quality = 0;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = FRAME_COUNT;
		scDesc.Scaling = DXGI_SCALING_STRETCH;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		ComPtr<IDXGISwapChain1> swapChain1;
		HRESULT hr = mDeviceService->GetFactory()->CreateSwapChainForHwnd(
			mCmdQueue.Get(),
			WindowHandle,
			&scDesc,
			nullptr,
			nullptr,
			&swapChain1);

		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		hr = swapChain1.As(&mSwapChain);
		if (FAILED(hr)) return false;

		return true;
	}

	bool DX12Context::InitializeBackBufferHeap()
	{
		ID3D12Device* device = mDeviceService->GetDevice();

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = FRAME_COUNT;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;

		HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
			mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		const UINT rtvIncSize =
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (UINT i = 0; i < FRAME_COUNT; ++i)
		{
			hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrames[i].BackBuffer));
			if (FAILED(hr))
			{
				// TODO:ログ出力
				return false;
			}
			device->CreateRenderTargetView(mFrames[i].BackBuffer.Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvIncSize;
		}

		return true;
	}

	bool DX12Context::InitializeDepthHeap(UINT Width, UINT Height)
	{
		ID3D12Device* device = mDeviceService->GetDevice();

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;

		HRESULT hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap));
		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		D3D12_RESOURCE_DESC depthDesc = {};
		depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Width = static_cast<UINT64>(Width);
		depthDesc.Height = Height;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;

		hr = device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&mDepthBuffer));

		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		device->CreateDepthStencilView(
			mDepthBuffer.Get(), nullptr, mDsvHeap->GetCPUDescriptorHandleForHeapStart());

		return true;
	}

	bool DX12Context::InitializeFence()
	{
		HRESULT hr = mDeviceService->GetDevice()->CreateFence(
			0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));

		if (FAILED(hr))
		{
			// TODO:ログ出力
			return false;
		}

		mNextFenceValue = 1;

		mWaitForGPUEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mWaitForGPUEventHandle == nullptr) return false;

		for (UINT i = 0; i < FRAME_COUNT; ++i)
		{
			mFrames[i].FenceValue = 0;
		}

		return true;
	}


}