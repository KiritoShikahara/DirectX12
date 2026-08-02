#pragma once

#include <Utility/Export/Export.h>
#include <graphics/Color/Color.h>
#include <array>
#include "Dx12Type.h"

namespace graphics
{
    class DX12Device;

    /// <summary>
    /// DX12描画コンテキストクラス
    /// </summary>
    class ENGINE_API DX12Context
    {
    public:
        DX12Context();
        virtual ~DX12Context();

        /// <summary>
        /// 初期化
        /// </summary>
        bool Initialize(DX12Device* pDevice, HWND WindowHandle, UINT Width, UINT Height);

        /// <summary>
        /// 終了処理
        /// </summary>
        bool Finalize();

        /// <summary>
        /// フレーム描画開始
        /// </summary>
        void BeginRendering();

        /// <summary>
        /// 画面フリップ
        /// </summary>
        void Flip();

        /// <summary>
        /// GPU完了待機
        /// </summary>
        void WaitForGPU();

        /// <summary>
        /// ビューポート設定
        /// </summary>
        void SetViewPort(ID3D12GraphicsCommandList* cmdList,
            float Width, float Height, float x = 0.0f, float y = 0.0f);

        /// <summary>
        /// メインレンダーターゲット再セット
        /// </summary>
        void RestoreMainRenderTarget(ID3D12GraphicsCommandList* cmdList);

        /// <summary>
        /// コマンドリスト取得
        /// </summary>
        ID3D12GraphicsCommandList* GetCommandList(eRenderChannel channel);

        /// <summary>
        /// コマンドアロケーター取得
        /// </summary>
        ID3D12CommandAllocator* GetCommandAllocator(eRenderChannel channel);

        /// <summary>
        /// コマンドキュー取得
        /// </summary>
        ID3D12CommandQueue* GetCommandQueue();

        /// <summary>
        /// アップロードプール取得
        /// </summary>
        D3D12MA::Pool* GetMAUploadPool();

        /// <summary>
        /// 現在フレームインデックス取得
        /// </summary>
        UINT GetCurrentFrameIndex() const;

        /// <summary>
        /// スクリーン横幅取得
        /// </summary>
        UINT GetWidth() const { return mWidth; }

        /// <summary>
        /// スクリーン縦幅取得
        /// </summary>
        UINT GetHeight() const { return mHeight; }

    private:
        /// <summary>
        /// コマンドオブジェクト初期化
        /// </summary>
        bool InitializeCommandObjects();

        /// <summary>
        /// スワップチェイン初期化
        /// </summary>
        bool InitializeSwapChain(HWND WindowHandle, UINT Width, UINT Height);

        /// <summary>
        /// バックバッファヒープ初期化
        /// </summary>
        bool InitializeBackBufferHeap();

        /// <summary>
        /// 深度ヒープ初期化
        /// </summary>
        bool InitializeDepthHeap(UINT Width, UINT Height);

        /// <summary>
        /// フェンス初期化
        /// </summary>
        bool InitializeFence();

        /// <summary>
        /// 現在のRTVハンドル取得
        /// </summary>
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtvHandle() const;

        /// <summary>
        /// DSVハンドル取得
        /// </summary>
        D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

        /// <summary>
        /// リソースバリア発行
        /// </summary>
        static void Barrier(
            ID3D12GraphicsCommandList* cmdList,
            ID3D12Resource* resource,
            D3D12_RESOURCE_STATES before,
            D3D12_RESOURCE_STATES after);

        /// <summary>
        /// フレームごとのリソース
        /// </summary>
        struct FrameResource
        {
            /// <summary>バックバッファ</summary>
            Resource BackBuffer = nullptr;
            /// <summary>フェンス値</summary>
            UINT64   FenceValue = 0;
            /// <summary>アップロードプール</summary>
            MAPool   UploadPool = nullptr;
            /// <summary>アロケーター配列</summary>
            std::array<CmdAlloc, CHANNEL_COUNT> Allocators{};
            /// <summary>コマンドリスト配列</summary>
            std::array<CmdList, CHANNEL_COUNT> CmdLists{};
        };

        /// <summary>DX12デバイスサービス</summary>
        DX12Device* mDeviceService = nullptr;

        /// <summary>スワップチェイン</summary>
        SwapChain   mSwapChain;
        /// <summary>コマンドキュー</summary>
        CmdQueue    mCmdQueue;

        /// <summary>フレームリソース配列</summary>
        std::array<FrameResource, graphics::FRAME_COUNT> mFrames;

        /// <summary>深度バッファ</summary>
        Resource    mDepthBuffer;
        /// <summary>RTVヒープ</summary>
        Heap        mRtvHeap;
        /// <summary>DSVヒープ</summary>
        Heap        mDsvHeap;

        /// <summary>フェンス</summary>
        Fence       mFence;

        /// <summary>GPU待機イベント</summary>
        HANDLE      mWaitForGPUEventHandle = nullptr;
        /// <summary>次のフェンス値</summary>
        UINT64      mNextFenceValue = 1;
        /// <summary>フレームインデックス</summary>
        UINT        mFrameIndex = 0;

        /// <summary>RTVインクリメントサイズ</summary>
        UINT        mRtvIncrementSize = 0;

        /// <summary>クリアカラー</summary>
        Color       mClearColor;
        /// <summary>バックバッファフォーマット</summary>
        DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

        /// <summary>横幅</summary>
        UINT mWidth = 0;
        /// <summary>縦幅</summary>
        UINT mHeight = 0;
    };

} // namespace graphics