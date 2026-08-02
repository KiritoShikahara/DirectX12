#pragma once

#include"MouseButton.h"

#include <DirectXMath.h>
#include <array>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace sys
{
	class Mouse
	{
    public:
        Mouse();
        ~Mouse() = default;

        Mouse(const Mouse&) = delete;
        Mouse& operator=(const Mouse&) = delete;

        /// <summary>
        /// ウィンドウメッセージから入力状態を更新する
        /// </summary>
        /// <returns>マウス関連のメッセージなら true</returns>
        bool ProcessEvent(UINT message, WPARAM wParam, LPARAM lParam);

        /// <summary>フレームの終わりに呼ぶ。前フレーム状態を保存し、差分を計算する</summary>
        void Update();

        /// <summary>このフレームで押された</summary>
        [[nodiscard]] bool IsPressed(eMouseButton button) const;

        /// <summary>現在押されている</summary>
        [[nodiscard]] bool IsHeld(eMouseButton button) const;

        /// <summary>このフレームで離された</summary>
        [[nodiscard]] bool IsReleased(eMouseButton button) const;

        /// <summary>クライアント座標 (ピクセル)</summary>
        [[nodiscard]] DirectX::XMFLOAT2 GetPosition()      const;

        /// <summary>前フレームからの移動量</summary>
        [[nodiscard]] DirectX::XMFLOAT2 GetDeltaPosition() const;

        /// <summary>このフレームのホイール量 (1.0 = 1ノッチ)</summary>
        [[nodiscard]] float GetWheel() const;

        /// <summary>何かしらのマウス入力があったかどうか</summary>
        [[nodiscard]] bool IsAnyInput() const;
    private:
        [[nodiscard]] static eMouseButton ToMouseButton(UINT message, WPARAM wParam);
        [[nodiscard]] static bool         IsValid(eMouseButton button);
        void SetButtonState(eMouseButton button, bool isDown);

        static constexpr int kButtonCount = static_cast<int>(eMouseButton::Count);

        std::array<bool, kButtonCount> mCurrButtons{};
        std::array<bool, kButtonCount> mPrevButtons{};

        DirectX::XMFLOAT2 mPosition{ 0.0f, 0.0f };
        DirectX::XMFLOAT2 mPrevPosition{ 0.0f, 0.0f };
        DirectX::XMFLOAT2 mDeltaPosition{ 0.0f, 0.0f };

        float mWheel = 0.0f;
	};
}

