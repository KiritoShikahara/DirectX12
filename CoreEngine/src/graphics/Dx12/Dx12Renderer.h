#pragma once

#include <Utility/Export/Export.h>
#include <memory>

namespace graphics
{
    class DX12Device;
    class DX12Context;

    /// <summary>
    /// 描画系の上位管理クラス
    /// DX12Context(低レベルDX12操作)を所有・管理し、
    /// ゲームループから呼ばれる描画フローの窓口を提供する。
    /// 
    /// 使用例:
    ///   renderer.Initialize(device, hwnd, width, height);
    ///   // メインループ
    ///   renderer.BeginFrame();
    ///   renderer.GetContext()->GetCommandList()->...  // 描画コマンド発行
    ///   renderer.EndFrame();
    ///   // 終了
    ///   renderer.Finalize();
    /// </summary>
    class ENGINE_API DX12Renderer
    {
    public:
        DX12Renderer();
        virtual ~DX12Renderer();

        // コピー・ムーブ禁止(GPU リソースを所有するため)
        DX12Renderer(const DX12Renderer&) = delete;
        DX12Renderer& operator=(const DX12Renderer&) = delete;
        DX12Renderer(DX12Renderer&&) = delete;
        DX12Renderer& operator=(DX12Renderer&&) = delete;

        /// <summary>
        /// 初期化
        /// DX12Context を生成し描画に必要な全リソースを確保する。
        /// </summary>
        /// <param name="pDevice">初期化済みの DX12Device</param>
        /// <param name="WindowHandle">描画先ウィンドウのハンドル</param>
        /// <param name="Width">スクリーン横幅</param>
        /// <param name="Height">スクリーン縦幅</param>
        /// <returns>true:成功</returns>
        bool Initialize(DX12Device* pDevice, HWND WindowHandle, UINT Width, UINT Height);

        /// <summary>
        /// 終了処理
        /// 全GPU コマンドの完了を待ってからリソースを解放する。
        /// </summary>
        /// <returns>true:成功</returns>
        bool Finalize();

        /// <summary>
        /// フレーム開始
        /// バックバッファのクリア・レンダーターゲット設定・ビューポート設定を行う。
        /// </summary>
        void BeginFrame();

        /// <summary>
        /// フレーム終了
        /// コマンドを GPU へ送信し画面をフリップする。
        /// </summary>
        void EndFrame();

        /// <summary>
        /// GPU コマンドの完了を待機する(リソース解放前などに使用)
        /// </summary>
        void WaitForGPU();

        /// <summary>
        /// 管理している DX12Context を取得する
        /// 描画コマンドの発行などに使用する。
        /// </summary>
        /// <returns>DX12Context へのポインタ(nullptr の場合は未初期化)</returns>
        DX12Context* GetContext() const;

        /// <summary>
        /// 初期化済みかどうか
        /// </summary>
        bool IsInitialized() const;

    private:
        /// <summary>所有する描画コンテキスト</summary>
        std::unique_ptr<DX12Context> mContext;

        /// <summary>描画対象のスクリーン横幅(ビューポート設定に使用)</summary>
        UINT mWidth = 0;
        /// <summary>描画対象のスクリーン縦幅(ビューポート設定に使用)</summary>
        UINT mHeight = 0;

        /// <summary>初期化済みフラグ</summary>
        bool mIsInitialized = false;
    };

} // namespace graphics