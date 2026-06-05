#include "pch.h"
#include "Window.h"

#include"WindowContext.h"

#include<system/Input/InputManager.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //	ImGui用プロシージャー用処理呼び出し
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }
    // TODO:Input
	if (sys::InputManager::Get().ProcessEvent(msg, wParam, lParam))
	{
		return true;
	}

    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}


namespace sys
{
    Window::Window()
        : mHandle(nullptr)
        , mWindowClass{}
        , mWidth(0)
        , mHeight(0)
        , mVirtualWidth(0)
        , mVirtualHeight(0)
        , mIsQuitRequested(false)
        , mIsCursorVisible(true)
    {

    }
    Window::~Window()
    {
        if (mHandle != nullptr)
        {
            ::UnregisterClass(mWindowClass.lpszClassName, mWindowClass.hInstance);
        }
    }


    /// <summary>
    /// ウィンドウの生成
    /// </summary>
    /// <param name="context"></param>
    /// <returns></returns>
    bool Window::Initialize(const WindowContext& context)
    {
        // ウィンドウサイズ
        mWidth = context.Width;
        mHeight = context.Height;
        mVirtualWidth = context.VirtualWidth;
        mVirtualHeight = context.VirtualHeight;

        // ウィンドウクラス設定
        mWindowClass.cbSize = sizeof(WNDCLASSEX);
        mWindowClass.style = CS_HREDRAW | CS_VREDRAW;
        mWindowClass.lpfnWndProc = WndProc;
        mWindowClass.cbClsExtra = 0;
        mWindowClass.cbWndExtra = 0;
        mWindowClass.hInstance = GetModuleHandle(nullptr);
        mWindowClass.hIcon = LoadIcon(nullptr, IDC_ARROW);
        mWindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        mWindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        mWindowClass.lpszMenuName = nullptr;
        mWindowClass.lpszClassName = context.Title.c_str();
        mWindowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        // ウィンドウクラスの登録
        if (::RegisterClassExW(&mWindowClass) == FALSE)
        {
            // TODO:ログ出力
            return false;
        }

        // ウィンドウスタイルと表示座標・領域の設定
        DWORD style = context.IsFullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;

        if (context.IsFullScreen)
        {
            // モニターの解像度を物理サイズとして上書き
            mWidth = GetSystemMetrics(SM_CXSCREEN);
            mHeight = GetSystemMetrics(SM_CYSCREEN);
            x = 0;
            y = 0;
        }
        else
        {
            // クライアント領域（中身）を指定サイズにする
            RECT wr = { 0, 0, mWidth, mHeight };
            ::AdjustWindowRect(&wr, style, FALSE);
            mWidth = wr.right - wr.left;
            mHeight = wr.bottom - wr.top;
        }

        // ウィンドウの生成
        mHandle = ::CreateWindowExW(
            0,
            mWindowClass.lpszClassName,
            context.Title.c_str(),
            style,
            x, y,
            mWidth, mHeight,
            nullptr,
            nullptr,
            mWindowClass.hInstance,
            nullptr
        );

        if (mHandle == nullptr)
        {
			// TODO:ログ出力
            return false;
        }

        // 表示とフォーカス
        ::ShowWindow(mHandle, SW_SHOW);
        ::UpdateWindow(mHandle);
        ::SetForegroundWindow(mHandle);
        ::SetFocus(mHandle);

        // カーソルの初期状態を適用
        SetCursorVisible(context.ShowCursor);

		// TODO:ログ出力

        return true;
    }

    /// <summary>
    /// メッセージループの処理
    /// </summary>
    void Window::ProcessMessages()
    {
        MSG msg = {};
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                mIsQuitRequested = true;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    /// <summary>
    /// 終了フラグの確認
    /// </summary>
    /// <returns>true:App終了</returns>
    bool Window::IsQuitRequested() const
    {
        return mIsQuitRequested;
    }

    /// <summary>
    /// ウィンドウハンドル取得
    /// </summary>
    /// <returns></returns>
    HWND Window::GetHWND() const
    {
        return mHandle;
    }

    /// <summary>
    /// カーソルの表示・非表示を変更する
    /// </summary>
    /// <param name="isVisible">true:表示する</param>
    void Window::SetCursorVisible(bool isVisible)
    {
        if (mIsCursorVisible == isVisible) return;

        mIsCursorVisible = isVisible;
        if (isVisible)
        {
            // カウンタが +(表示) になるまで呼び出す
            while (::ShowCursor(TRUE) < 0);
        }
        else
        {
            // カウンタが -(非表示) になるまで呼び出す
            while (::ShowCursor(FALSE) >= 0);
        }
    }

    /// <summary>
    /// カーソルの表示状態かどうかの判定
    /// </summary>
    /// <returns>true:表示中</returns>
    bool Window::IsCursorVisible() const
    {
        return mIsCursorVisible;
    }

    /*
    * サイズ・スケール取得
    */
    int Window::GetWidth() const { return mWidth; }
    int Window::GetHeight() const { return mHeight; }
    int Window::GetVirtualWidth() const { return mVirtualWidth; }
    int Window::GetVirtualHeight() const { return mVirtualHeight; }
    float Window::GetScaleX() const
    {
        return static_cast<float>(mWidth) / static_cast<float>(mVirtualWidth);
    }
    float Window::GetScaleY() const
    {
        return static_cast<float>(mHeight) / static_cast<float>(mVirtualHeight);
    }
}