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
    /// スワップチェインを使ったフレーム描画ループを管理する。
    /// デバイス層(DX12Device)に依存する。
    /// DX12Rendererによって所有・管理される。
    /// </summary>
    class ENGINE_API DX12Context
    {
    public:
        DX12Context();
        virtual ~DX12Context();

        /// <summary>
        /// 初期化
        /// </summary>
        /// <param name="pDevice">初期化済みの DX12Device</param>
        /// <param name="WindowHandle">対象ウィンドウのハンドル</param>
        /// <param name="Width">スクリーン横幅</param>
        /// <param name="Height">スクリーン縦幅</param>
        /// <returns>true:成功</returns>
        bool Initialize(DX12Device* pDevice, HWND WindowHandle, UINT Width, UINT Height);

        /// <summary>
        /// 終了処理
        /// </summary>
        /// <returns>true:成功</returns>
        bool Finalize();

        /// <summary>
        /// フレーム描画の開始
        /// (バックバッファのクリア・レンダーターゲット設定)
        /// </summary>
        void BeginRendering();

        /// <summary>
        /// 画面のフリップ(コマンド送信・Present)
        /// </summary>
        void Flip();

        /// <summary>
        /// 全GPU コマンドの完了を待機する
        /// </summary>
        void WaitForGPU();

        /// <summary>
        /// ビューポートとシザー矩形の設定
        /// </summary>
        void SetViewPort(float Width, float Height, float x = 0.0f, float y = 0.0f);

        /// <summary>
        /// 描画用コマンドリストの取得
        /// </summary>
        ID3D12GraphicsCommandList* GetCommandList();

        /// <summary>
        /// 現在フレームのコマンドアロケーターの取得
        /// </summary>
        ID3D12CommandAllocator* GetCommandAllocator();

        /// <summary>
        /// コマンドキューの取得
        /// </summary>
        ID3D12CommandQueue* GetCommandQueue();

        /// <summary>
        /// 現在フレームのD3D12MAアップロードプールの取得
        /// </summary>
        D3D12MA::Pool* GetMAUploadPool();

        /// <summary>
        /// 現在フレームのインデックスの取得
        /// </summary>
        UINT GetCurrentFrameIndex() const;

    private:
        bool InitializeCommandObjects();
        bool InitializeSwapChain(HWND WindowHandle, UINT Width, UINT Height);
        bool InitializeBackBufferHeap();
        bool InitializeDepthHeap(UINT Width, UINT Height);
        bool InitializeFence();

        /// <summary>
        /// フレームごとのリソースまとめ
        /// </summary>
        struct FrameResource
        {
            /// <summary>コマンドリストの記録に使う専用の領域。実行後はリセット必須</summary>
            CmdAlloc Allocator = nullptr;
            /// <summary>実際に色を書き込まれるバックバッファテクスチャ</summary>
            Resource BackBuffer = nullptr;
            /// <summary>このフレームのGPU完了を確認するためのフェンス値</summary>
            UINT64   FenceValue = 0;
            /// <summary>このフレーム用のアップロードプール</summary>
            MAPool   UploadPool = nullptr;
        };

        /// <summary>DX12Deviceへの参照(ライフタイムの管理はサービス側が行う)</summary>
        DX12Device* mDeviceService = nullptr;

        /// <summary>フロント・バックバッファの入れ替え</summary>
        SwapChain   mSwapChain;
        /// <summary>完了したコマンドをGPUへ送り出すキュー</summary>
        CmdQueue    mCmdQueue;
        /// <summary>GPUへの命令を記録するコマンドリスト</summary>
        CmdList     mCmdList;

        /// <summary>フレームごとのリソース配列</summary>
        std::array<FrameResource, graphics::FRAME_COUNT> mFrames;

        /// <summary>深度バッファリソース(前後関係の判断に使う)</summary>
        Resource    mDepthBuffer;
        /// <summary>RTV用ディスクリプタヒープ</summary>
        Heap        mRtvHeap;
        /// <summary>DSV用ディスクリプタヒープ</summary>
        Heap        mDsvHeap;

        /// <summary>CPUとGPUの同期用フェンス</summary>
        Fence       mFence;

        /// <summary>GPU待ちイベントハンドル</summary>
        HANDLE      mWaitForGPUEventHandle = nullptr;
        /// <summary>次にSignalする値</summary>
        UINT64      mNextFenceValue = 1;
        /// <summary>現在フレームのインデックス</summary>
        UINT        mFrameIndex = 0;

        /// <summary>背景クリア色</summary>
        Color       mClearColor;
        /// <summary>バックバッファのフォーマット</summary>
        DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    };

} // namespace graphics