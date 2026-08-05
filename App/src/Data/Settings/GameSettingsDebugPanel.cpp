#include "apppch.h"
#include "GameSettingsDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Settings/GameSettingsData.h>

namespace debug
{
	GameSettingsDebugPanel::GameSettingsDebugPanel(std::string debugKey)
		: mDebugKey(std::move(debugKey))
	{
#if DEV_TOOL_ENABLED
		data::EnsureGameSettingsLoaded();
		auto& mgr = data::ConfigRegistry::Get().GetManager<data::GameSettingsData>();
		mEditor = std::make_unique<data::ConfigEditor<data::GameSettingsData>>(mgr);

		sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
	}

	GameSettingsDebugPanel::~GameSettingsDebugPanel()
	{
#if DEV_TOOL_ENABLED
		sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
	}

#if DEV_TOOL_ENABLED
	void GameSettingsDebugPanel::Draw()
	{
		// Save時、音量2項目はAudioManagerへの反映(ApplyGameSettings)を伴わないと次回起動まで音に反映されないため、
		// このパネルで編集した値は保存の都度ここで反映する
		mEditor->Draw("Game Settings");
		::data::ApplyGameSettings();
	}
#else
	void GameSettingsDebugPanel::Draw() {}
#endif
}
