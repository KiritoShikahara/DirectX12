#pragma once

#include"KeyCode.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include<array>

namespace sys
{
	class Keyboard
	{
	public:
		Keyboard();
		virtual ~Keyboard() = default;

		// コピー・ムーブ禁止 (ユニークなデバイス状態を持つため)
		Keyboard(const Keyboard&) = delete;
		Keyboard& operator=(const Keyboard&) = delete;

		/// <summary>
		/// ウィンドウメッセージから入力状態を更新する
		/// </summary>
		/// <returns>キーボード関連のメッセージなら true</returns>
		bool ProcessEvent(UINT message, WPARAM vkCode);

		/// <summary>フレームの終わりに呼ぶ。前フレーム状態を保存する</summary>
		void Update();

		/// <summary>このフレームで押された (前フレームは離れていた)</summary>
		[[nodiscard]] bool IsPressed(eKeyCode keyCode) const;

		/// <summary>現在押されている</summary>
		[[nodiscard]] bool IsHeld(eKeyCode keyCode) const;

		/// <summary>このフレームで離された (前フレームは押されていた)</summary>
		[[nodiscard]] bool IsReleased(eKeyCode keyCode) const;

		/// <summary>いずれかのキーが押されているか</summary>
		[[nodiscard]] bool IsAnyKeyHeld() const;

		/// <summary>ImGui デバッグ表示 (InputManager::ImGuiUpdate から呼ぶ)</summary>
		void ImGuiUpdate() const;
	private:
		/// <summary>WPARAMの仮想キーコードを eKeyCode に変換する</summary>
		static eKeyCode ToKeyCode(WPARAM vkCode);

		/// <summary>キーコードが有効範囲内か確認する</summary>
		[[nodiscard]] static bool IsValid(eKeyCode keyCode);

		[[nodiscard]] static const char* KeyCodeToString(eKeyCode keyCode);

		/// <summary>キー状態を更新するヘルパー</summary>
		void SetKeyState(WPARAM vkCode, bool isDown);

		static constexpr int kKeyCount = static_cast<int>(sys::eKeyCode::Count);
		std::array<bool, kKeyCount> mCurrKeys{};
		std::array<bool, kKeyCount> mPrevKeys{};
	};
}


