#include"pch.h"
#include "Mouse.h"

namespace sys
{
    Mouse::Mouse()
    {
        mCurrButtons.fill(false);
        mPrevButtons.fill(false);
    }

    // ウィンドウメッセージ処理
    bool Mouse::ProcessEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        // マウス関連メッセージなら座標を更新
        if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
        {
            mPosition.x = static_cast<float>(GET_X_LPARAM(lParam));
            mPosition.y = static_cast<float>(GET_Y_LPARAM(lParam));
        }

        switch (message)
        {
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: case WM_XBUTTONDOWN:
            SetButtonState(ToMouseButton(message, wParam), true);
            return true;

        case WM_LBUTTONUP: case WM_RBUTTONUP:
        case WM_MBUTTONUP: case WM_XBUTTONUP:
            SetButtonState(ToMouseButton(message, wParam), false);
            return true;

        case WM_MOUSEWHEEL:
            mWheel += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam))
                / static_cast<float>(WHEEL_DELTA);
            return true;

        case WM_MOUSEMOVE:
            return true;

        default:
            return false;
        }
    }

    // フレーム更新
    void Mouse::Update()
    {
        mDeltaPosition.x = mPosition.x - mPrevPosition.x;
        mDeltaPosition.y = mPosition.y - mPrevPosition.y;
        mPrevPosition = mPosition;
        mPrevButtons = mCurrButtons;
        mWheel = 0.0f;
    }

    // 状態クエリ
    bool Mouse::IsPressed(eMouseButton button) const
    {
        if (!IsValid(button)) return false;
        const int idx = static_cast<int>(button);
        return mCurrButtons[idx] && !mPrevButtons[idx];
    }

    bool Mouse::IsHeld(eMouseButton button) const
    {
        if (!IsValid(button)) return false;
        return mCurrButtons[static_cast<int>(button)];
    }

    bool Mouse::IsReleased(eMouseButton button) const
    {
        if (!IsValid(button)) return false;
        const int idx = static_cast<int>(button);
        return !mCurrButtons[idx] && mPrevButtons[idx];
    }

    DirectX::XMFLOAT2 Mouse::GetPosition() const
    {
        return mPosition;
    }

    DirectX::XMFLOAT2 Mouse::GetDeltaPosition() const
    {
        return mDeltaPosition;
    }

    float Mouse::GetWheel() const
    {
        return mWheel;
    }

    // Privateヘルパー
    void Mouse::SetButtonState(eMouseButton button, bool isDown)
    {
        if (IsValid(button))
        {
            mCurrButtons[static_cast<int>(button)] = isDown;
        }
    }

    bool Mouse::IsValid(eMouseButton button)
    {
        return button > eMouseButton::Unknown && button < eMouseButton::Count;
    }

    eMouseButton Mouse::ToMouseButton(UINT message, WPARAM wParam)
    {
        switch (message)
        {
        case WM_LBUTTONDOWN: case WM_LBUTTONUP:
            return eMouseButton::Left;

        case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            return eMouseButton::Right;

        case WM_MBUTTONDOWN: case WM_MBUTTONUP:
            return eMouseButton::Middle;

        case WM_XBUTTONDOWN: case WM_XBUTTONUP:
            return (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                ? eMouseButton::XButton1
                : eMouseButton::XButton2;

        default:
            return eMouseButton::Unknown;
        }
    }

    bool Mouse::IsAnyInput() const
    {
        for (int i = 0; i < kButtonCount; ++i)
        {
            if (mCurrButtons[i]) return true;
        }

        // ホイール操作
        if (mWheel != 0.0f) return true;

        // 座標の移動（Update() 前なので mPosition と mPrevPosition を直接比較する）
        constexpr float kMoveEpsilon = 0.5f; // ピクセル単位。わずかな揺れは無視する
        return std::abs(mPosition.x - mPrevPosition.x) > kMoveEpsilon
            || std::abs(mPosition.y - mPrevPosition.y) > kMoveEpsilon;
    }
}