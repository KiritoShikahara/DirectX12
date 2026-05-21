#include"pch.h"
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

    // -----------------------------------------------------------------------
 //  ボタンクエリ
 // -----------------------------------------------------------------------
    bool Pad::IsPressed(ePadButton button) const
    {
        const int li = static_cast<int>(TriggerSide::Left);
        const int ri = static_cast<int>(TriggerSide::Right);

        switch (button)
        {
        case ePadButton::L2:
            return mCurrTrigger[li] && !mPrevTrigger[li];
        case ePadButton::R2:
            return mCurrTrigger[ri] && !mPrevTrigger[ri];
        default:
        {
            const uint32_t mask = static_cast<uint32_t>(ToWgiButton(button));
            return ((mCurrButtons & mask) != 0) && ((mPrevButtons & mask) == 0);
        }
        }
    }

    bool Pad::IsHeld(ePadButton button) const
    {
        switch (button)
        {
        case ePadButton::L2:
            return mCurrTrigger[static_cast<int>(TriggerSide::Left)];
        case ePadButton::R2:
            return mCurrTrigger[static_cast<int>(TriggerSide::Right)];
        default:
        {
            const uint32_t mask = static_cast<uint32_t>(ToWgiButton(button));
            return (mCurrButtons & mask) != 0;
        }
        }
    }

    bool Pad::IsReleased(ePadButton button) const
    {
        const int li = static_cast<int>(TriggerSide::Left);
        const int ri = static_cast<int>(TriggerSide::Right);

        switch (button)
        {
        case ePadButton::L2:
            return !mCurrTrigger[li] && mPrevTrigger[li];
        case ePadButton::R2:
            return !mCurrTrigger[ri] && mPrevTrigger[ri];
        default:
        {
            const uint32_t mask = static_cast<uint32_t>(ToWgiButton(button));
            return ((mCurrButtons & mask) == 0) && ((mPrevButtons & mask) != 0);
        }
        }
    }

    // -----------------------------------------------------------------------
    //  スティック取得
    // -----------------------------------------------------------------------
    DirectX::XMFLOAT2 Pad::GetLeftStick3D() const
    {
        return mLeftStick;
    }

    DirectX::XMFLOAT2 Pad::GetRightStick3D() const
    {
        return mRightStick;
    }

    DirectX::XMFLOAT2 Pad::GetLeftStick2D() const
    {
        return { mLeftStick.x, -mLeftStick.y };
    }

    DirectX::XMFLOAT2 Pad::GetRightStick2D() const
    {
        return { mRightStick.x, -mRightStick.y };
    }

    // -----------------------------------------------------------------------
    //  ImGui デバッグ表示
    // -----------------------------------------------------------------------
    void Pad::ImGuiUpdate()
    {
#ifdef _DEBUG
        ImGui::Separator();
        ImGui::PushID(this);

        const auto lStick = GetLeftStick2D();
        const auto rStick = GetRightStick2D();

        ImGui::Text("Left Stick:  (%.2f, %.2f)", lStick.x, lStick.y);
        ImGui::ProgressBar((lStick.x + 1.0f) / 2.0f, ImVec2(100, 8), "X");
        ImGui::ProgressBar((lStick.y + 1.0f) / 2.0f, ImVec2(100, 8), "Y");

        ImGui::Text("Right Stick: (%.2f, %.2f)", rStick.x, rStick.y);
        ImGui::ProgressBar((rStick.x + 1.0f) / 2.0f, ImVec2(100, 8), "X");
        ImGui::ProgressBar((rStick.y + 1.0f) / 2.0f, ImVec2(100, 8), "Y");

        // スティックの円表示
        constexpr float kRadius = 40.0f;
        auto DrawStickCircle = [&](const DirectX::XMFLOAT2& stick, ImU32 dotColor)
            {
                const ImVec2 cursor = ImGui::GetCursorScreenPos();
                const ImVec2 center(cursor.x + kRadius, cursor.y + kRadius);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddCircle(center, kRadius, IM_COL32(200, 200, 200, 100), 32, 2.0f);
                const ImVec2 pos(center.x + stick.x * kRadius,
                    center.y + stick.y * kRadius);
                dl->AddCircleFilled(pos, 5.0f, dotColor);
                ImGui::Dummy(ImVec2(kRadius * 2.0f, kRadius * 2.0f));
            };

        DrawStickCircle(lStick, IM_COL32(255, 100, 100, 255));
        DrawStickCircle(rStick, IM_COL32(100, 255, 100, 255));

        ImGui::Text("Triggers:");
        ImGui::Text("L: %.2f", mLeftTrigger);
        ImGui::ProgressBar(mLeftTrigger, ImVec2(100, 10));
        ImGui::Text("R: %.2f", mRightTrigger);
        ImGui::ProgressBar(mRightTrigger, ImVec2(100, 10));

        ImGui::Spacing();
        ImGui::Text("Buttons:");
        ImGui::BeginChild("buttons_child", ImVec2(0, 0), true);

        for (int i = 0; i < static_cast<int>(ePadButton::Count); ++i)
        {
            const auto btn = static_cast<ePadButton>(i);
            const bool held = IsHeld(btn);
            if (held) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 120, 120, 255));
            ImGui::Text("%s", PadButtonToString(btn).c_str());
            if (held) ImGui::PopStyleColor();
        }

        ImGui::EndChild();
        ImGui::PopID();
#endif // _DEBUG
    }
}