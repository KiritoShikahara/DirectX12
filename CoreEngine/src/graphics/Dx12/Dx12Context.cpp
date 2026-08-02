#include "pch.h"
#include "DX12Context.h"
#include "Dx12Device.h"
#include "RenderContext.h"

#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include <graphics/Profiler/GpuProfiler.h>

namespace graphics
{
    // コンストラクタ
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

    // デストラクタ
    DX12Context::~DX12Context()
    {
    }

    // 初期化処理
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
        if (!InitializeFence())            return false;

        GpuProfiler::Get().Initialize(mDeviceService->GetDevice(), mCmdQueue.Get());

        return true;
    }

    // 終了処理
    bool DX12Context::Finalize()
    {
        WaitForGPU();

        GpuProfiler::Get().Finalize();

        if (mWaitForGPUEventHandle != nullptr)
        {
            CloseHandle(mWaitForGPUEventHandle);
            mWaitForGPUEventHandle = nullptr;
        }

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

        mSwapChain.Reset();
        mCmdQueue.Reset();

        mFence.Reset();

        return true;
    }

    // フレーム描画開始
    void DX12Context::BeginRendering()
    {
        mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

        RenderContext::Get().SetFrameIndex(mFrameIndex);

        auto& frame = mFrames[mFrameIndex];

        if (mFence->GetCompletedValue() < frame.FenceValue)
        {
            mFence->SetEventOnCompletion(frame.FenceValue, mWaitForGPUEventHandle);
            WaitForSingleObject(mWaitForGPUEventHandle, INFINITE);
        }

        GpuProfiler::Get().BeginFrame(mFrameIndex);

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentRtvHandle();
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvHandle();

        ID3D12DescriptorHeap* heaps[] = { GDescriptorHeapManager::Get().GetNativeHeap() };

        for (uint32_t i = 0; i < CHANNEL_COUNT; ++i)
        {
            frame.Allocators[i]->Reset();
            frame.CmdLists[i]->Reset(frame.Allocators[i].Get(), nullptr);

            ID3D12GraphicsCommandList* cmdList = frame.CmdLists[i].Get();

            cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
            cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
            SetViewPort(cmdList,
                static_cast<float>(mWidth), static_cast<float>(mHeight));

            GpuProfiler::Get().BeginChannel(cmdList, static_cast<eRenderChannel>(i));
        }

        ID3D12GraphicsCommandList* preCmdList = GetCommandList(eRenderChannel::Pre);

        Barrier(preCmdList, frame.BackBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        preCmdList->ClearRenderTargetView(rtvHandle, mClearColor.GetRawPointer(), 0, nullptr);
        preCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    // 画面フリップ処理
    void DX12Context::Flip()
    {
        auto& frame = mFrames[mFrameIndex];

        Barrier(GetCommandList(eRenderChannel::Post), frame.BackBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);

        auto& gpuProfiler = GpuProfiler::Get();
        for (uint32_t i = 0; i < CHANNEL_COUNT; ++i)
        {
            gpuProfiler.EndChannel(frame.CmdLists[i].Get(), static_cast<eRenderChannel>(i));
        }

        gpuProfiler.ResolveFrame(GetCommandList(eRenderChannel::Post));

        ID3D12CommandList* cmdLists[CHANNEL_COUNT] = {};
        for (uint32_t i = 0; i < CHANNEL_COUNT; ++i)
        {
            frame.CmdLists[i]->Close();
            cmdLists[i] = frame.CmdLists[i].Get();
        }

        mCmdQueue->ExecuteCommandLists(CHANNEL_COUNT, cmdLists);

        mSwapChain->Present(1, 0);

        mNextFenceValue++;
        frame.FenceValue = mNextFenceValue;
        mCmdQueue->Signal(mFence.Get(), mNextFenceValue);
    }

    // GPU完了待機
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

    // ビューポート設定
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

    // メインレンダーターゲット再セット
    void DX12Context::RestoreMainRenderTarget(ID3D12GraphicsCommandList* cmdList)
    {
        if (cmdList == nullptr) return;

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentRtvHandle();
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvHandle();

        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        SetViewPort(cmdList,
            static_cast<float>(mWidth), static_cast<float>(mHeight));
    }

    // コマンドリスト取得
    ID3D12GraphicsCommandList* DX12Context::GetCommandList(eRenderChannel channel)
    {
        return mFrames[mFrameIndex].CmdLists[static_cast<uint32_t>(channel)].Get();
    }

    // コマンドアロケーター取得
    ID3D12CommandAllocator* DX12Context::GetCommandAllocator(eRenderChannel channel)
    {
        return mFrames[mFrameIndex].Allocators[static_cast<uint32_t>(channel)].Get();
    }

    // コマンドキュー取得
    ID3D12CommandQueue* DX12Context::GetCommandQueue()
    {
        return mCmdQueue.Get();
    }

    // アップロードプール取得
    D3D12MA::Pool* DX12Context::GetMAUploadPool()
    {
        return mFrames[mFrameIndex].UploadPool.Get();
    }

    // 現在フレームインデックス取得
    UINT DX12Context::GetCurrentFrameIndex() const
    {
        return mFrameIndex;
    }

    // 現在のRTVハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE DX12Context::GetCurrentRtvHandle() const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(mFrameIndex) * mRtvIncrementSize;
        return handle;
    }

    // DSVハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE DX12Context::GetDsvHandle() const
    {
        return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    }

    // リソースバリア発行
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

    // コマンドオブジェクト初期化
    bool DX12Context::InitializeCommandObjects()
    {
        ID3D12Device* device = mDeviceService->GetDevice();
        HRESULT hr = S_OK;

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

                mFrames[f].CmdLists[c]->Close();
            }
        }

        return true;
    }

    // スワップチェイン初期化
    bool DX12Context::InitializeSwapChain(HWND WindowHandle, UINT Width, UINT Height)
    {
        DXGI_SWAP_CHAIN_DESC1 scDesc = {};
        scDesc.Width = Width;
        scDesc.Height = Height;
        scDesc.Format = mFormat;
        scDesc.Stereo = FALSE;
        scDesc.SampleDesc.Count = 1;
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

    // バックバッファヒープ初期化
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

    // 深度ヒープ初期化
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

    // フェンス初期化
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