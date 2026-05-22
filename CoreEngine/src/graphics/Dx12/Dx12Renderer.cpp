#include "pch.h"
#include "DX12Renderer.h"
#include "DX12Context.h"
#include "Dx12Device.h"

namespace graphics
{
    DX12Renderer::DX12Renderer()
        : mContext(nullptr)
        , mWidth(0)
        , mHeight(0)
        , mIsInitialized(false)
    {
    }

    DX12Renderer::~DX12Renderer()
    {
        // Finalize が呼ばれていない場合の安全弁
        if (mIsInitialized)
        {
            Finalize();
        }
    }

    bool DX12Renderer::Initialize(DX12Device* pDevice, HWND WindowHandle, UINT Width, UINT Height)
    {
        if (pDevice == nullptr)     return false;
        if (mIsInitialized)         return false;  // 二重初期化禁止

        mWidth = Width;
        mHeight = Height;

        // DX12Context を生成・初期化
        mContext = std::make_unique<DX12Context>();
        if (!mContext->Initialize(pDevice, WindowHandle, Width, Height))
        {
            mContext.reset();
            return false;
        }

        mIsInitialized = true;
        return true;
    }

    bool DX12Renderer::Finalize()
    {
        if (!mIsInitialized) return false;

        if (mContext)
        {
            mContext->Finalize();
            mContext.reset();
        }

        mIsInitialized = false;
        return true;
    }

    void DX12Renderer::BeginFrame()
    {
        if (!mIsInitialized) return;

        // コンテキストに描画開始を委譲
        mContext->BeginRendering();

        // 毎フレームのビューポートをスクリーン全体に設定
        // 必要に応じて呼び出し側から SetViewPort で上書き可能
        mContext->SetViewPort(static_cast<float>(mWidth), static_cast<float>(mHeight));
    }

    void DX12Renderer::EndFrame()
    {
        if (!mIsInitialized) return;

        mContext->Flip();
    }

    void DX12Renderer::WaitForGPU()
    {
        if (!mIsInitialized) return;

        mContext->WaitForGPU();
    }

    DX12Context* DX12Renderer::GetContext() const
    {
        return mContext.get();
    }

    bool DX12Renderer::IsInitialized() const
    {
        return mIsInitialized;
    }

} // namespace graphics