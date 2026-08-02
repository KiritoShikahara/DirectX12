#include "apppch.h"
#include "StatUpgradeDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/StatUpgrade/StatUpgradeData.h>

namespace debug
{
    StatUpgradeDebugPanel::StatUpgradeDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::StatUpgradeData>();
        mInspector = std::make_unique<data::DataInspector<data::StatUpgradeData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    StatUpgradeDebugPanel::~StatUpgradeDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void StatUpgradeDebugPanel::Draw()
    {
        mInspector->Draw("StatUpgrade Master");
    }
#else
    void StatUpgradeDebugPanel::Draw() {}
#endif
}
