#include "pch.h"
#include "DX12Context.h"
#include "Dx12Device.h"
#include "RenderContext.h"

#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include <graphics/Profiler/GpuProfiler.h>

namespace graphics
{
    DX12Context::DX12Context()
        : mDeviceService(nullptr)
        , mSwapChain(nullptr)
        , mCmdQueue(nullptr)
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

    bool DX12Context::Initialize(DX12Device* pDevice, HWND WindowHandle, UINT Width, UINT Height)
    {
        if (pDevice == nullptr) return false;
        mDeviceService = pDevice;
        mWidth = Width;
        mHeight = Height;

        if (!InitializeCommandObjects())  return false;
        if (!InitializeSwapChain(WindowHandle, Width, Height)) return false;
        if (!InitializeBackBufferHeap())  return false;
        if (!InitializeDepthHeap(Width, Height)) return false;
        if (!InitializeFence())           return false;

        // GPU計測はタイムスタンプ未対応環境では無効化されるだけなので、
        // 失敗しても初期化全体は続行する
        GpuProfiler::Get().Initialize(mDeviceService->GetDevice(), mCmdQueue.Get());

        return true;
    }

    bool DX12Context::Finalize()
    {
        WaitForGPU();

        // GPUの完了を待った後に解放する(クエリヒープを実行中に破棄しないため)
        GpuProfiler::Get().Finalize();

        if (mWaitForGPUEventHandle != nullptr)
        {
            CloseHandle(mWaitForGPUEventHandle);
            mWaitForGPUEventHandle = nullptr;
        }

        // 生成と逆順で解放する
        for (auto& frame : mFrames)
        {
            for (auto& cmdList : frame.CmdLists)   cmdList.Reset();
            for (auto& alloc : frame.Allocators)   alloc.Reset();
            frame.BackBuffer.Reset();
            frame.UploadPool.Reset();
        }

        mDepthBuffer.Reset();
        mRtvHeap.Reset();
        mDsvHeap.Reset();

        mSwapChain.Reset();  // CommandQueue より先に解放
        mCmdQueue.Reset();

        mFence.Reset();

        return true;
    }

    // -----------------------------------------------------------------------
    //  フレーム描画
    // -----------------------------------------------------------------------

    void DX12Context::BeginRendering()
    {
        // 次に描画するバックバッファのインデックスを取得
        mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

        // リングバッファ(StructuredBuffer / ConstantBuffer / VertexBuffer)の
        // 切り替えに使われるため、コマンド記録より前に必ず通知する
        RenderContext::Get().SetFrameIndex(mFrameIndex);

        auto& frame = mFrames[mFrameIndex];

        // このフレームのGPU処理が終了していなければ待機(ストール防止)
        if (mFence->GetCompletedValue() < frame.FenceValue)
        {
            mFence->SetEventOnCompletion(frame.FenceValue, mWaitForGPUEventHandle);
            WaitForSingleObject(mWaitForGPUEventHandle, INFINITE);
        }

        // 前フレーム分のGPU計測結果を回収する。直前のフェンス待機で
        // このフレームインデックスのGPU完了は保証済みのため、追加の待機は発生しない
        GpuProfiler::Get().BeginFrame(mFrameIndex);

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentRtvHandle();
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvHandle();

        ID3D12DescriptorHeap* heaps[] = { GDescriptorHeapManager::Get().GetNativeHeap() };

        // 全チャネルのコマンド記録を開始する。
        // DescriptorHeap / RTV / DSV / Viewport はコマンドリスト単位の状態のため、
        // チャネルごとに毎フレーム設定し直す必要がある。
        for (uint32_t i = 0; i < CHANNEL_COUNT; ++i)
        {
            frame.Allocators[i]->Reset();
            frame.CmdLists[i]->Reset(frame.Allocators[i].Get(), nullptr);

            ID3D12GraphicsCommandList* cmdList = frame.CmdLists[i].Get();

            cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
            cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
            SetViewPort(cmdList,
                static_cast<float>(mWidth), static_cast<float>(mHeight));

            // チャネル先頭でGPUタイムスタンプを打つ(終了側はFlipで打つ)
            GpuProfiler::Get().BeginChannel(cmdList, static_cast<eRenderChannel>(i));
        }

        // Pre チャネル: バックバッファを PRESENT → RENDER_TARGET へ遷移してクリアする
        ID3D12GraphicsCommandList* preCmdList = GetCommandList(eRenderChannel::Pre);

        Barrier(preCmdList, frame.BackBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        preCmdList->ClearRenderTargetView(rtvHandle, mClearColor.GetRawPointer(), 0, nullptr);
        preCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    void DX12Context::Flip()
    {
        auto& frame = mFrames[mFrameIndex];

        // Post チャネル: バックバッファを RENDER_TARGET → PRESENT へ遷移
        Barrier(GetCommandList(eRenderChannel::Post), frame.BackBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);

        // 各チャネル末尾でGPUタイムスタンプを打つ。
        // 記録は並列でも、この時点では全ワーカーの記録が完了している(呼び出し側でWaitAll済み)
        auto& gpuProfiler = GpuProfiler::Get();
        for (uint32_t i = 0; i < CHANNEL_COUNT; ++i)
        {
            gpuProfiler.EndChannel(frame.CmdLists[i].Get(), static_cast<eRenderChannel>(i));
        }

        // クエリ結果の書き出しは、全チャネルのタイムスタンプを打った後に
        // 最後のチャネル(Post)へ積む(GPU実行順で最後になるため全結果が確定している)
        gpuProfiler.ResolveFrame(GetCommandList(eRenderChannel::Post));

        // 全チャネルを確定する
        ID3D12CommandList* cmdLists[CHANNEL_COUNT] = {};
        for (uint32_t i = 0; i < CHANNEL_COUNT; ++i)
        {
            frame.CmdLists[i]->Close();
            cmdLists[i] = frame.CmdLists[i].Get();
        }

        // 記録は並列でも構わないが、GPU への投入はチャネルの宣言順(= 描画順)で行う
        mCmdQueue->ExecuteCommandLists(CHANNEL_COUNT, cmdLists);

        // 画面の切り替え
        mSwapChain->Present(1, 0);

        // このフレームの完了フェンス値を記録
        mNextFenceValue++;
        frame.FenceValue = mNextFenceValue;
        mCmdQueue->Signal(mFence.Get(), mNextFenceValue);
    }

    void DX12Context::WaitForGPU()
    {
        if (mCmdQueue == nullptr || mFence == nullptr) return;

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

    // -----------------------------------------------------------------------
    //  設定
    // -----------------------------------------------------------------------

    void DX12Context::SetViewPort(
        ID3D12GraphicsCommandList* cmdList,
        float Width, float Height, float x, float y)
    {
        if (cmdList == nullptr) return;

        D3D12_VIEWPORT viewport = { x, y, Width, Height, 0.0f, 1.0f };
        D3D12_RECT     scissor = {
            static_cast<LONG>(x),
            static_cast<LONG>(y),
            static_cast<LONG>(x + Width),
            static_cast<LONG>(y + Height) };

        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissor);
    }

    void DX12Context::RestoreMainRenderTarget(ID3D12GraphicsCommandList* cmdList)
    {
        if (cmdList == nullptr) return;

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentRtvHandle();
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvHandle();

        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        SetViewPort(cmdList,
            static_cast<float>(mWidth), static_cast<float>(mHeight));
    }

    // -----------------------------------------------------------------------
    //  アクセサ
    // -----------------------------------------------------------------------

    ID3D12GraphicsCommandList* DX12Context::GetCommandList(eRenderChannel channel)
    {
        return mFrames[mFrameIndex].CmdLists[static_cast<uint32_t>(channel)].Get();
    }

    ID3D12CommandAllocator* DX12Context::GetCommandAllocator(eRenderChannel channel)
    {
        return mFrames[mFrameIndex].Allocators[static_cast<uint32_t>(channel)].Get();
    }

    ID3D12CommandQueue* DX12Context::GetCommandQueue()
    {
        return mCmdQueue.Get();
    }

    D3D12MA::Pool* DX12Context::GetMAUploadPool()
    {
        return mFrames[mFrameIndex].UploadPool.Get();
    }

    UINT DX12Context::GetCurrentFrameIndex() const
    {
        return mFrameIndex;
    }

    // -----------------------------------------------------------------------
    //  Private ヘルパー
    // -----------------------------------------------------------------------

    D3D12_CPU_DESCRIPTOR_HANDLE DX12Context::GetCurrentRtvHandle() const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(mFrameIndex) * mRtvIncrementSize;
        return handle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DX12Context::GetDsvHandle() const
    {
        return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    }

    void DX12Context::Barrier(
        ID3D12GraphicsCommandList* cmdList,
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES before,
        D3D12_RESOURCE_STATES after)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
    }

    // -----------------------------------------------------------------------
    //  Private 初期化
    // -----------------------------------------------------------------------

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
            DEBUG_LOG(sys::eLogLevel::Error, "DX12Context: Failed to create CommandQueue.");
            return false;
        }

        D3D12MA::Allocator* maAllocator = mDeviceService->GetMAAllocator();

        for (uint32_t f = 0; f < FRAME_COUNT; ++f)
        {
            // フレームごとのアップロードプール
            D3D12MA::POOL_DESC poolDesc = {};
            poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            poolDesc.Flags = D3D12MA::POOL_FLAG_ALGORITHM_LINEAR;

            hr = maAllocator->CreatePool(&poolDesc, &mFrames[f].UploadPool);
            if (FAILED(hr))
            {
                DEBUG_LOG(sys::eLogLevel::Error,
                    "DX12Context: Failed to create UploadPool. frame={}", f);
                return false;
            }

            // チャネルごとのアロケーターとコマンドリスト
            for (uint32_t c = 0; c < CHANNEL_COUNT; ++c)
            {
                hr = device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(&mFrames[f].Allocators[c]));
                if (FAILED(hr))
                {
                    DEBUG_LOG(sys::eLogLevel::Error,
                        "DX12Context: Failed to create CommandAllocator. frame={} channel={}", f, c);
                    return false;
                }

                hr = device->CreateCommandList(
                    0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                    mFrames[f].Allocators[c].Get(), nullptr,
                    IID_PPV_ARGS(&mFrames[f].CmdLists[c]));
                if (FAILED(hr))
                {
                    DEBUG_LOG(sys::eLogLevel::Error,
                        "DX12Context: Failed to create CommandList. frame={} channel={}", f, c);
                    return false;
                }

                // 最初は記録しない状態にしておく
                mFrames[f].CmdLists[c]->Close();
            }
        }

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
            mCmdQueue.Get(), WindowHandle, &scDesc, nullptr, nullptr, &swapChain1);
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "DX12Context: Failed to create SwapChain.");
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
            DEBUG_LOG(sys::eLogLevel::Error, "DX12Context: Failed to create RTV heap.");
            return false;
        }

        // インクリメントサイズをキャッシュ（毎フレームの取得を避ける）
        mRtvIncrementSize =
            device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (UINT i = 0; i < FRAME_COUNT; ++i)
        {
            hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrames[i].BackBuffer));
            if (FAILED(hr))
            {
                DEBUG_LOG(sys::eLogLevel::Error,
                    "DX12Context: Failed to get back buffer. index={}", i);
                return false;
            }
            device->CreateRenderTargetView(mFrames[i].BackBuffer.Get(), nullptr, rtvHandle);
            rtvHandle.ptr += mRtvIncrementSize;
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
            DEBUG_LOG(sys::eLogLevel::Error, "DX12Context: Failed to create DSV heap.");
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
            &heapProp, D3D12_HEAP_FLAG_NONE,
            &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue, IID_PPV_ARGS(&mDepthBuffer));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "DX12Context: Failed to create depth buffer.");
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
            DEBUG_LOG(sys::eLogLevel::Error, "DX12Context: Failed to create Fence.");
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

} // namespace graphics