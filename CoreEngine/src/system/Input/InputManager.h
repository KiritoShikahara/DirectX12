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
		/// アクションバインディング定義 
		/// </summary>
		struct ActionBinding
		{
			eKeyCode     key = eKeyCode::Count;     // Count = 未割り当て
			ePadButton   pad = ePadButton::Count;   // Count = 未割り当て
			eMouseButton mouse = eMouseButton::Count; // Count = 未割り当て
		};

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


		/// <summary>
		/// 移動入力 (正規化済み、3D空間向け Y = 前が正)
		/// </summary>
		[[nodiscard]] DirectX::XMFLOAT2 GetMoveAxis() const;

		/// <summary>
		/// 視点入力 (マウス移動 + 右スティック の合成、正規化済み)
		/// </summary>
		[[nodiscard]] DirectX::XMFLOAT2 GetLookAxis() const;


		/// <summary>
		/// アクション名とデバイスコードの組を登録する
		/// 同名のアクションは上書きされる
		/// </summary>
		void AddAction(const std::string& actionName, const struct ActionBinding& bind);

		[[nodiscard]] bool IsActionPressed(const std::string& actionName) const;
		[[nodiscard]] bool IsActionHeld(const std::string& actionName) const;
		[[nodiscard]] bool IsActionReleased(const std::string& actionName) const;

	private:
		std::unordered_map<std::string, ActionBinding> mActionMaps;

		std::unique_ptr<PadManager> mPadManager;
		std::unique_ptr<Mouse>      mMouse;
		std::unique_ptr<Keyboard>   mKeyboard;

		bool mIsInitialized = false;
	};
}

#define INPUT_MANAGER sys::InputManager::Get()
#define INPUT_PAD sys::InputManager::Get().GetPadManager()
#define INPUT_MOUSE sys::InputManager::Get().GetMouse()
#define INPUT_KEYBOARD sys::InputManager::Get().GetKeyboard()