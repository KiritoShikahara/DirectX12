#pragma once
#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>

#include<Windows.h>

namespace sys
{
	struct WindowContext;

	/// <summary>
	/// ウィンドウ関係の管理
	/// </summary>
	class ENGINE_API Window : public utility::Singleton<Window>
	{
		SINGLETON_FRIEND(Window);
	public:
		SINGLETON_ACCESSOR(Window);

		/// <summary>
		/// ウィンドウの生成
		/// </summary>
		/// <param name="context"></param>
		/// <returns></returns>
		bool Initialize(const WindowContext& context);

		/// <summary>
		/// メッセージループの処理
		/// </summary>
		void ProcessMessages();

		/// <summary>
		/// 終了フラグの確認
		/// </summary>
		/// <returns>true:App終了</returns>
		bool IsQuitRequested() const;

		///<summary>
		///アプリケーションの終了を要求する、WM_CLOSEを送り通常の終了経路に乗せる
		///</summary>
		void RequestQuit();

		/// <summary>
		/// ウィンドウハンドル取得
		/// </summary>
		/// <returns></returns>
		HWND GetHWND() const;

		/// <summary>
		/// カーソルの表示・非表示を変更する
		/// </summary>
		/// <param name="isVisible">true:表示する</param>
		void SetCursorVisible(bool isVisible);

		/// <summary>
		/// カーソルの表示状態かどうかの判定
		/// </summary>
		/// <returns>true:表示中</returns>
		bool IsCursorVisible()const;


		//	実際のウィンドウのサイズ
		int GetWidth() const;
		int GetHeight() const;

		//	内部の仮想ウィンドウサイズ
		int GetVirtualWidth() const;
		int GetVirtualHeight() const;

		//	仮想サイズから実サイズのスケール比
		float GetScaleX() const;
		float GetScaleY() const;

	private:

		Window();
		virtual ~Window();

		/// <summary>
		/// ウィンドウクラス
		/// </summary>
		WNDCLASSEX mWindowClass;

		/// <summary>
		/// ウィンドウのハンドル
		/// </summary>
		HWND mHandle;

		/// <summary>
		/// 実際のウィンドウの横サイズ
		/// </summary>
		int mWidth;
		/// <summary>
		/// 実際のウィンドウの縦サイズ
		/// </summary>
		int mHeight;
		/// <summary>
		/// 仮想の横サイズ
		/// </summary>
		int mVirtualWidth;
		/// <summary>
		/// 仮想の縦サイズ
		/// </summary>
		int mVirtualHeight;

		/// <summary>
		/// ウィンドウを閉じるかどうか
		/// true:閉じる false:続行！
		/// </summary>
		bool mIsQuitRequested;

		/// <summary>
		/// カーソルを表示するかどうか。Windowsは標準で表示されるようなのでtrueから
		/// </summary>
		bool mIsCursorVisible;

	};
}

