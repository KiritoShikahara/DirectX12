#include "apppch.h"
#include "EnemyStatusDebugPanel.h"

#include"EnemyStatusComponent.h"

namespace debug
{
	void EnemyStatusDebugPanel::RegisterDebugUI(const std::string& DebugKey)
	{
#ifdef _DEBUG
		sys::ImGuiManager::Get().AddDebugUI(Draw, mDebugKey);
#endif
	}

	void EnemyStatusDebugPanel::UnregisterDebugUI()
	{
#ifdef _DEBUG
		sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
	}

	void EnemyStatusDebugPanel::Draw()
	{
	}

	void EnemyStatusDebugPanel::ApplyRowToEnemies(const data::EnemyData& row)
	{
	}
}