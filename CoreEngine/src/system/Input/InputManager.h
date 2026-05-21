#pragma once

#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <unordered_map>

#include<System/Input/Keyboard/KeyBoard.h>
#include<system/Input/Mouse/Mouse.h>
#include<system/Input/PAD/PadManager.h>

namespace sys
{
	/// <summary>
	/// 入力管理クラス
	/// </summary>
	class InputManager : public utility::Singleton<InputManager>
	{
		SINGLETON_CLASS(InputManager);
	public:
		SINGLETON_ACCESSOR(InputManager);


		/// <summary>
		/// 初期化：デフォルトのキーボード・マウス・ゲームパッドをセットアップする
		/// </summary>
		/// <returns>true:成功</returns>
		bool Initialize();

		/// <summary>
		/// 状態更新
		/// </summary>
		void Update();

		/// <summary>
		/// WndProc から転送する
		/// </summary>
		/// <returns>入力関連メッセージなら true</returns>
		bool ProcessEvent(UINT message, WPARAM wParam, LPARAM lParam);


		/*
		* デバイス直接アクセス
		*/
		[[nodiscard]] Keyboard* GetKeyboard()   const { return mKeyboard.get(); }
		[[nodiscard]] Mouse* GetMouse()       const { return mMouse.get(); }
		[[nodiscard]] PadManager* GetPadManager()  const { return mPadManager.get(); }

		struct ActionBinding
		{
			eKeyCode     key = eKeyCode::Count;     // Count = 未割り当て
			ePadButton   pad = ePadButton::Count;   // Count = 未割り当て
			eMouseButton mouse = eMouseButton::Count; // Count = 未割り当て
		};

	private:
		std::unordered_map<std::string, ActionBinding> mActionMaps;

		std::unique_ptr<PadManager> mPadManager;
		std::unique_ptr<Mouse>      mMouse;
		std::unique_ptr<Keyboard>   mKeyboard;
	};
}