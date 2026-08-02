#pragma once

#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include<System/Input/Keyboard/KeyBoard.h>
#include<system/Input/Mouse/Mouse.h>
#include<system/Input/PAD/PadManager.h>

namespace sys
{

	/// <summary>
	/// 最後に入力があった入力デバイスの種別。
	/// エイム方式の切り替えなど、デバイスに応じた挙動の分岐に使う。
	/// </summary>
	enum class eInputDevice
	{
		KeyboardMouse,
		Pad,
	};

	/// <summary>
	/// 入力管理クラス
	/// </summary>
	class InputManager : public utility::Singleton<InputManager>
	{
		SINGLETON_CLASS(InputManager);
	public:
		SINGLETON_ACCESSOR(InputManager);


		/// <summary>
		/// アクションバインディング定義。
		/// 1つのアクションに対して、各デバイスで複数のコードを割り当てられる。
		/// 判定は配列内・デバイス間ともに OR。
		/// </summary>
		struct ActionBinding
		{
			std::vector<eKeyCode>     Keys;
			std::vector<ePadButton>   Pads;
			std::vector<eMouseButton> Mouses;
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
		void AddAction(const std::string& actionName, const ActionBinding& bind);

		// 入力判定
		[[nodiscard]] bool IsActionPressed(const std::string& actionName) const;
		[[nodiscard]] bool IsActionHeld(const std::string& actionName) const;
		[[nodiscard]] bool IsActionReleased(const std::string& actionName) const;

		/// <summary>
		/// IsActionPressedと同様だが、マウスの割り当ては無視する(キーボード/パッドのみ判定する)。
		/// パーク選択のように、画面上のクリックで誤って決定してしまうと困る画面向け。
		/// </summary>
		[[nodiscard]] bool IsActionPressedExcludingMouse(const std::string& actionName) const;

		/// <summary>
		/// 最後に入力があったデバイスを取得する。
		/// どちらにも入力が無いフレームでは前回の値を維持する。
		/// </summary>
		[[nodiscard]] eInputDevice GetLastInputDevice() const { return mLastInputDevice; }


		/// <summary>
		/// マウス座標を仮想解像度基準（Window::GetVirtualWidth/Height）に変換して返す。
		/// CameraSystem::ScreenPointToRay 等はこの座標系を前提とするため、
		/// 生の GetPosition()（実ウィンドウのピクセル座標）ではなくこちらを使うこと。
		/// </summary>
		[[nodiscard]] DirectX::XMFLOAT2 GetMouseVirtualPosition() const;

	private:
		/// <summary>各デバイスの入力有無を見て mLastInputDevice を更新する</summary>
		void UpdateLastInputDevice();

		/// <summary>最後に入力があったデバイス</summary>
		eInputDevice mLastInputDevice = eInputDevice::KeyboardMouse;

		std::unordered_map<std::string, ActionBinding> mActionMaps;

		std::unique_ptr<PadManager> mPadManager;
		std::unique_ptr<Mouse>      mMouse;
		std::unique_ptr<Keyboard>   mKeyboard;

		bool mIsInitialized = false;
	};
}

#define INPUT_MANAGER ::sys::InputManager::Get()
#define INPUT_PAD ::sys::InputManager::Get().GetPadManager()
#define INPUT_MOUSE ::sys::InputManager::Get().GetMouse()
#define INPUT_KEYBOARD ::sys::InputManager::Get().GetKeyboard()