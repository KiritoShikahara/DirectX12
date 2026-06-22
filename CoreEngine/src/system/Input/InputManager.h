#pragma once

#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <initializer_list>

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
		/// アクションバインディング定義。
		/// 1つのアクションに対して、各デバイスで複数のコードを割り当てられる。
		/// （例: Key = {Up, W} のように、上矢印と W のどちらでも判定が通る）
		/// 判定は各デバイス内で OR、デバイス間でも OR。
		/// </summary>
		struct ActionBinding
		{
			std::vector<eKeyCode>     Keys;
			std::vector<ePadButton>   Pads;
			std::vector<eMouseButton> Mouses;

			ActionBinding() = default;

			/// <summary>
			/// 旧形式 { eKeyCode, ePadButton, eMouseButton } との互換用コンストラクタ。
			/// 各デバイス1コードのみ割り当てたい場合に簡潔に書ける。
			/// eKeyCode::Count / ePadButton::Count / eMouseButton::Count は「未割り当て」として無視する。
			/// </summary>
			ActionBinding(eKeyCode key, ePadButton pad = ePadButton::Count, eMouseButton mouse = eMouseButton::Count)
			{
				if (key != eKeyCode::Count)     Keys.push_back(key);
				if (pad != ePadButton::Count)   Pads.push_back(pad);
				if (mouse != eMouseButton::Count) Mouses.push_back(mouse);
			}

			/// <summary>
			/// 複数コードを直接指定するコンストラクタ。
			/// 例: ActionBinding({eKeyCode::Up, eKeyCode::W}, {ePadButton::DPadUp})
			/// </summary>
			ActionBinding(
				std::initializer_list<eKeyCode> keys,
				std::initializer_list<ePadButton> pads = {},
				std::initializer_list<eMouseButton> mouses = {})
				: Keys(keys.begin(), keys.end())
				, Pads(pads.begin(), pads.end())
				, Mouses(mouses.begin(), mouses.end())
			{
			}
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

		/// <summary>
		/// 既存のアクションに対して、キーコードを1つ追加する。
		/// アクションが未登録の場合は何もしない。
		/// </summary>
		void AddKeyToAction(const std::string& actionName, eKeyCode key);

		/// <summary>
		/// 既存のアクションに対して、パッドボタンを1つ追加する。
		/// アクションが未登録の場合は何もしない。
		/// </summary>
		void AddPadToAction(const std::string& actionName, ePadButton pad);

		/// <summary>
		/// 既存のアクションに対して、マウスボタンを1つ追加する。
		/// アクションが未登録の場合は何もしない。
		/// </summary>
		void AddMouseToAction(const std::string& actionName, eMouseButton mouse);

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