#include "apppch.h"
#include "GameDebugSettings.h"

#include <Utility/config/DebugConfig.h>
#include <Data/Settings/GameSettingsData.h>

#include <cstdlib>
#include <cstring>

namespace debug
{
    void GameDebugSettings::Initialize()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().AddDebugUI([this]() { ImGuiWindow(); }, "DebugSettings");
#endif
    }

    void GameDebugSettings::Finalize()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI("DebugSettings");
#endif
    }

    void GameDebugSettings::ParseCommandLine(const char* commandLine)
    {
#if DEV_TOOL_ENABLED
        if (commandLine == nullptr) return;

        const std::string args(commandLine);

        if (args.find("--godmode") != std::string::npos)
        {
            mPlayerInvincible = true;
        }

        // --godmode指定時は無人計測が止まらないよう自動選択も有効にする
        if (args.find("--autoperk") != std::string::npos || mPlayerInvincible)
        {
            mAutoSelectPerk = true;
        }

        // --autoexit=SECONDS
        const std::string exitKey = "--autoexit=";
        const size_t exitPos = args.find(exitKey);
        if (exitPos != std::string::npos)
        {
            mAutoExitSeconds = static_cast<float>(std::atof(args.c_str() + exitPos + exitKey.size()));
        }

        // --seed=N、乱数を固定して負荷を再現可能にする
        const std::string seedKey = "--seed=";
        const size_t seedPos = args.find(seedKey);
        if (seedPos != std::string::npos)
        {
            mFixedSeed = true;
            mRandomSeed = static_cast<unsigned int>(std::strtoul(args.c_str() + seedPos + seedKey.size(), nullptr, 10));
        }
#else
        (void)commandLine;
#endif
    }

    std::mt19937 GameDebugSettings::MakeRandomEngine() const
    {
#if DEV_TOOL_ENABLED
        if (mFixedSeed)
        {
            return std::mt19937{ mRandomSeed };
        }
#endif
        return std::mt19937{ std::random_device{}() };
    }

    void GameDebugSettings::ImGuiWindow()
    {
#if DEV_TOOL_ENABLED
        if (ImGui::Begin("Debug Settings"))
        {
            ImGui::TextUnformatted("Player");
            ImGui::Checkbox("Invincible (god mode)", &mPlayerInvincible);
            ImGui::TextDisabled("Keeps the fight going so heavy-load");
            ImGui::TextDisabled("performance can be measured.");

            ImGui::Separator();
            ImGui::TextUnformatted("Automation");
            ImGui::Checkbox("Auto-select perks", &mAutoSelectPerk);
            ImGui::TextDisabled("Perk select pauses the game (TimeScale 0),");
            ImGui::TextDisabled("which blocks unattended benchmarking.");

            ImGui::Separator();
            ImGui::TextUnformatted("Audio");

            // ゲーム内メニューと同じGameSettingsDataを直接編集する。保存はSaveボタンで明示的に行う
            auto& configReg = ::data::ConfigRegistry::Get();
            if (configReg.IsRegistered<::data::GameSettingsData>())
            {
                auto& settingsMgr = configReg.GetManager<::data::GameSettingsData>();
                auto& settings = settingsMgr.Get();

                bool changed = false;
                changed |= ImGui::SliderFloat("Master", &settings.MasterVolume, 0.0f, 1.0f);
                changed |= ImGui::SliderFloat("BGM", &settings.BgmVolume, 0.0f, 1.0f);
                changed |= ImGui::SliderFloat("SE", &settings.SeVolume, 0.0f, 1.0f);

                if (changed)
                {
                    settingsMgr.NotifyChanged();
                    ::data::ApplyGameSettings();
                }

                if (ImGui::Button("Save settings"))
                {
                    ::data::SaveGameSettings();
                }
                ImGui::SameLine();
                ImGui::TextDisabled("%s", settingsMgr.GetLastMessage().c_str());
            }
            else
            {
                ImGui::TextDisabled("(settings not loaded)");
            }

            ImGui::Separator();
            ImGui::TextDisabled("Command line: --godmode / --autoexit=SEC / --seed=N");
        }
        ImGui::End();
#endif
    }
}
