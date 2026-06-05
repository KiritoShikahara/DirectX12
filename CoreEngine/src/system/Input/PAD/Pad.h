#pragma once

#include <DirectXMath.h>
#include <winrt/windows.gaming.input.h>
#include <array>
#include <cstdint>
#include <string>
#include"PadButton.h"


namespace sys
{

	using Gamepad = winrt::Windows::Gaming::Input::Gamepad;


	/// <summary>
	/// 単一ゲームパッドの入力管理クラス
	/// デッドゾーン処理・トリガーのデジタル化・スティック値の提供を行う
	/// </summary>
	class Pad
	{
	public:
        /// <param name="gamepad">WGI Gamepad インスタンス</param>
        explicit Pad(
            const Gamepad& gamepad,
            float rightStickDeadZone = 0.1f,
            float leftStickDeadZone = 0.2f,
            float triggerDeadZone = 0.1f);

        ~Pad() = default;

        // Gamepad はコピーしても問題ないが Pad は状態を持つので明示的に管理
        Pad(const Pad&) = default;
        Pad& operator=(const Pad&) = default;
        Pad(Pad&&) = default;
        Pad& operator=(Pad&&) = default;

        /// <summary>
        /// 更新
        /// フレームの終わりに呼ぶ
        /// </summary>
        void Update();

        /*
        * ボタン入力クエリ
        */
        [[nodiscard]] bool IsPressed(ePadButton button) const;
        [[nodiscard]] bool IsHeld(ePadButton button) const;
        [[nodiscard]] bool IsReleased(ePadButton button) const;

        /*
        * スティック 3D
        */
        [[nodiscard]] DirectX::XMFLOAT2 GetLeftStick3D()  const;
        [[nodiscard]] DirectX::XMFLOAT2 GetRightStick3D() const;

        /*
        * スティック 2D
        */
        [[nodiscard]] DirectX::XMFLOAT2 GetLeftStick2D()  const;
        [[nodiscard]] DirectX::XMFLOAT2 GetRightStick2D() const;

        /*
        * トリガー：アナログ値で0.0 - 1.0
        */
        [[nodiscard]] float GetLeftTrigger()  const { return mLeftTrigger; }
        [[nodiscard]] float GetRightTrigger() const { return mRightTrigger; }
        
        /*
        * デッドゾーン
        */
        void SetLeftStickDeadZone(float v) { mLeftStickDeadZone = v; }
        void SetRightStickDeadZone(float v) { mRightStickDeadZone = v; }
        void SetTriggerDeadZone(float v) { mTriggerDeadZone = v; }

        /// <summary>
		/// 内部で保持している WGI Gamepad インスタンスを取得
        /// </summary>
        [[nodiscard]] Gamepad GetGamepad() const { return mGamepad; }

        /// <summary>
        /// ImGUiデバック表示
        /// </summary>
        void ImGuiUpdate();

    private:
        enum class TriggerSide : int { Left = 0, Right, Count };

        /// <summary>デッドゾーン以下の入力を 0.0 に丸める</summary>
        [[nodiscard]] static float ApplyDeadZone(float value, float deadZone);

        Gamepad mGamepad;

        DirectX::XMFLOAT2 mLeftStick{ 0.0f, 0.0f };
        DirectX::XMFLOAT2 mRightStick{ 0.0f, 0.0f };

        float mLeftTrigger = 0.0f;
        float mRightTrigger = 0.0f;

        float mLeftStickDeadZone = 0.2f;
        float mRightStickDeadZone = 0.1f;
        float mTriggerDeadZone = 0.1f;

        uint32_t mCurrButtons = 0;
        uint32_t mPrevButtons = 0;

        static constexpr int kTriggerCount = static_cast<int>(TriggerSide::Count);
        std::array<bool, kTriggerCount> mCurrTrigger{};
        std::array<bool, kTriggerCount> mPrevTrigger{};

	};

    /// <summary>デバッグ表示用のボタン名文字列</summary>
    [[nodiscard]] std::string PadButtonToString(ePadButton button);
}
