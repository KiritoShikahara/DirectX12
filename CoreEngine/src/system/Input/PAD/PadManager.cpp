#include"pch.h"
#include "PadManager.h"

#include <winrt/Windows.Foundation.Collections.h>

#include<Utility/config/DebugConfig.h>

namespace sys
{
    PadManager::PadManager()
    {
        namespace WGI = winrt::Windows::Gaming::Input;

        // 接続済みパッドを列挙
        const auto gamepads = WGI::Gamepad::Gamepads();
        for (uint32_t i = 0; i < gamepads.Size(); ++i)
        {
            mPads.emplace_back(gamepads.GetAt(i));
        }

        // 接続イベント (バックグラウンドスレッドから呼ばれる)
        WGI::Gamepad::GamepadAdded(
            [this](auto const&, WGI::Gamepad const& pad)
            {
                std::lock_guard lock(mMutex);
                mPads.emplace_back(pad);
            });

        // 切断イベント (バックグラウンドスレッドから呼ばれる)
        WGI::Gamepad::GamepadRemoved(
            [this](auto const&, WGI::Gamepad const& pad)
            {
                std::lock_guard lock(mMutex);
                std::erase_if(mPads, [&](const Pad& entry)
                    {
                        return entry.GetGamepad() == pad;
                    });
            });
    }

    /// <summary>
    /// フレーム更新
    /// </summary>
    void PadManager::Update()
    {
        std::lock_guard lock(mMutex);
        for (auto& pad : mPads)
        {
            pad.Update();
        }
    }

	// ボタン入力クエリ
    bool PadManager::IsPressed(ePadButton button, ePadIndex index) const
    {
        if (!IsValidButtonCode(button)) return false;
        std::lock_guard lock(mMutex);
        const uint8_t i = static_cast<uint8_t>(index);
        if (!IsValidIndex(i)) return false;
        return mPads[i].IsPressed(button);
    }

    bool PadManager::IsHeld(ePadButton button, ePadIndex index) const
    {
        if (!IsValidButtonCode(button)) return false;
        std::lock_guard lock(mMutex);
        const uint8_t i = static_cast<uint8_t>(index);
        if (!IsValidIndex(i)) return false;
        return mPads[i].IsHeld(button);
    }

    bool PadManager::IsReleased(ePadButton button, ePadIndex index) const
    {
        if (!IsValidButtonCode(button)) return false;
        std::lock_guard lock(mMutex);
        const uint8_t i = static_cast<uint8_t>(index);
        if (!IsValidIndex(i)) return false;
        return mPads[i].IsReleased(button);
    }

    // スティック取得
    DirectX::XMFLOAT2 PadManager::GetLeftStick3D(ePadIndex index) const
    {
        std::lock_guard lock(mMutex);
        const uint8_t i = static_cast<uint8_t>(index);
        return IsValidIndex(i) ? mPads[i].GetLeftStick3D() : DirectX::XMFLOAT2{ 0.0f, 0.0f };
    }

    DirectX::XMFLOAT2 PadManager::GetRightStick3D(ePadIndex index) const
    {
        std::lock_guard lock(mMutex);
        const uint8_t i = static_cast<uint8_t>(index);
        return IsValidIndex(i) ? mPads[i].GetRightStick3D() : DirectX::XMFLOAT2{ 0.0f, 0.0f };
    }

    DirectX::XMFLOAT2 PadManager::GetLeftStick2D(ePadIndex index) const
    {
        std::lock_guard lock(mMutex);
        const uint8_t i = static_cast<uint8_t>(index);
        return IsValidIndex(i) ? mPads[i].GetLeftStick2D() : DirectX::XMFLOAT2{ 0.0f, 0.0f };
    }

    DirectX::XMFLOAT2 PadManager::GetRightStick2D(ePadIndex index) const
    {
        std::lock_guard lock(mMutex);
        const uint8_t i = static_cast<uint8_t>(index);
        return IsValidIndex(i) ? mPads[i].GetRightStick2D() : DirectX::XMFLOAT2{ 0.0f, 0.0f };
    }

    // ユーティリティ
    std::vector<ePadIndex> PadManager::GetAvailableIndices() const
    {
        std::lock_guard lock(mMutex);
        std::vector<ePadIndex> result;
        const size_t count = std::min(mPads.size(), static_cast<size_t>(ePadIndex::Count));
        result.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            result.push_back(static_cast<ePadIndex>(i));
        }
        return result;
    }

    size_t PadManager::GetConnectedCount() const
    {
        std::lock_guard lock(mMutex);
        return mPads.size();
    }

    // Privateヘルパー
    bool PadManager::IsValidIndex(uint8_t index) const
    {
        return static_cast<size_t>(index) < mPads.size()
            && index < static_cast<uint8_t>(ePadIndex::Count);
    }

    bool PadManager::IsValidButtonCode(ePadButton button)
    {
        return button != ePadButton::Count;
    }

    bool PadManager::IsAnyInput() const
    {
        std::lock_guard lock(mMutex);
        for (const auto& pad : mPads)
        {
            if (pad.IsAnyInput()) return true;
        }
        return false;
    }

    // ImGuiデバック
    void PadManager::ImGuiUpdate()
    {
#ifdef ENABLE_INPUT_DEBUG_PAD
         // InputManager が CollapsingHeader を開いた内側でこの関数を呼ぶ
        std::lock_guard lock(mMutex);

        if (ImGui::Begin("Controller"))
        {
            if (mPads.empty())
            {
                ImGui::TextDisabled("(no gamepad connected)");
            }

            for (int i = 0; i < static_cast<int>(mPads.size()); ++i)
            {
                const std::string label = "Gamepad " + std::to_string(i);
                if (ImGui::TreeNode(label.c_str()))
                {
                    mPads[i].ImGuiUpdate();
                    ImGui::TreePop();
                }
            }
        }

        ImGui::End();
#endif // ENABLE_INPUT_DEBUG_PAD
    }
}