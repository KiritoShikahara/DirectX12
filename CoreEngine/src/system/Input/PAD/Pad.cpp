#include "Pad.h"

namespace sys
{
    /// <summary>
	/// ボタンを文字列に変換するユーティリティ
    /// </summary>
    std::string PadButtonToString(ePadButton button)
    {
        switch (button)
        {
        case ePadButton::A:         return "A";
        case ePadButton::B:         return "B";
        case ePadButton::X:         return "X";
        case ePadButton::Y:         return "Y";
        case ePadButton::DPadUp:    return "DPad Up";
        case ePadButton::DPadDown:  return "DPad Down";
        case ePadButton::DPadLeft:  return "DPad Left";
        case ePadButton::DPadRight: return "DPad Right";
        case ePadButton::L1:        return "L1";
        case ePadButton::R1:        return "R1";
        case ePadButton::L2:        return "L2";
        case ePadButton::R2:        return "R2";
        case ePadButton::L3:        return "L3";
        case ePadButton::R3:        return "R3";
        case ePadButton::Menu:      return "Menu";
        case ePadButton::View:      return "View";
        case ePadButton::Paddle1:   return "Paddle1";
        case ePadButton::Paddle2:   return "Paddle2";
        case ePadButton::Paddle3:   return "Paddle3";
        case ePadButton::Paddle4:   return "Paddle4";
        default:                    return "Unknown";
        }
    }


    /// <summary>
    /// コンストラクタ
    /// </summary>
    Pad::Pad(const Gamepad& gamepad,
        float rightStickDeadZone,
        float leftStickDeadZone,
        float triggerDeadZone)
        : mGamepad(gamepad)
        , mLeftStickDeadZone(leftStickDeadZone)
        , mRightStickDeadZone(rightStickDeadZone)
        , mTriggerDeadZone(triggerDeadZone)
    {
        mCurrTrigger.fill(false);
        mPrevTrigger.fill(false);
    }

    /// <summary>
    /// フレーム更新
    /// </summary>
    void Pad::Update()
    {
        mPrevButtons = mCurrButtons;
        mPrevTrigger = mCurrTrigger;

        const auto reading = mGamepad.GetCurrentReading();

        // ボタンビットフィールド
        mCurrButtons = static_cast<uint32_t>(reading.Buttons);

        // トリガー (デッドゾーン適用後にデジタル化)
        mLeftTrigger = ApplyDeadZone(reading.LeftTrigger, mTriggerDeadZone);
        mRightTrigger = ApplyDeadZone(reading.RightTrigger, mTriggerDeadZone);
        mCurrTrigger[static_cast<int>(TriggerSide::Left)] = (mLeftTrigger > 0.0f);
        mCurrTrigger[static_cast<int>(TriggerSide::Right)] = (mRightTrigger > 0.0f);

        // スティック
        mLeftStick.x = ApplyDeadZone(reading.LeftThumbstickX, mLeftStickDeadZone);
        mLeftStick.y = ApplyDeadZone(reading.LeftThumbstickY, mLeftStickDeadZone);
        mRightStick.x = ApplyDeadZone(reading.RightThumbstickX, mRightStickDeadZone);
        mRightStick.y = ApplyDeadZone(reading.RightThumbstickY, mRightStickDeadZone);
    }

    /// <summary>
    /// デッドゾーン適応
    /// </summary>
    float Pad::ApplyDeadZone(float value, float deadZone)
    {
        deadZone = std::max(deadZone, 0.01f); // 0 デッドゾーンによる誤動作を防止
        return (std::abs(value) < deadZone) ? 0.0f : value;
    }
}