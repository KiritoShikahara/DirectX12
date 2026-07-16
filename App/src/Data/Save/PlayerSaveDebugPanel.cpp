#include "apppch.h"
#include "PlayerSaveDebugPanel.h"

#include<Data/Save/PlayerSaveData.h>

namespace debug
{
	PlayerSaveDebugPanel::PlayerSaveDebugPanel(std::string debugKey)
		: mDebugKey(std::move(debugKey))
	{
#ifdef _DEBUG
		data::EnsurePlayerSaveDataLoaded();
		auto& mgr = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>();
		mEditor = std::make_unique<data::ConfigEditor<data::PlayerSaveData>>(mgr);

		sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
	}

	PlayerSaveDebugPanel::~PlayerSaveDebugPanel()
	{
#ifdef _DEBUG
		sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
	}

#ifdef _DEBUG
	void PlayerSaveDebugPanel::Draw()
	{
		mEditor->Draw("Player Save Data");
	}
#else
	void PlayerSaveDebugPanel::Draw() {}
#endif
}
