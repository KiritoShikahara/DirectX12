#include "apppch.h"
#include "PlayerSaveDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Save/PlayerSaveData.h>

namespace debug
{
	PlayerSaveDebugPanel::PlayerSaveDebugPanel(std::string debugKey)
		: mDebugKey(std::move(debugKey))
	{
#if DEV_TOOL_ENABLED
		data::EnsurePlayerSaveDataLoaded();
		auto& mgr = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>();
		mEditor = std::make_unique<data::ConfigEditor<data::PlayerSaveData>>(mgr);

		sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
	}

	PlayerSaveDebugPanel::~PlayerSaveDebugPanel()
	{
#if DEV_TOOL_ENABLED
		sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
	}

#if DEV_TOOL_ENABLED
	void PlayerSaveDebugPanel::Draw()
	{
		mEditor->Draw("Player Save Data");
	}
#else
	void PlayerSaveDebugPanel::Draw() {}
#endif
}
