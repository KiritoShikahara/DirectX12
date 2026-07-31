#include "apppch.h"
#include "BossDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Enemy/BossData.h>

namespace debug
{
    BossDebugPanel::BossDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::BossData>();
        mInspector = std::make_unique<data::DataInspector<data::BossData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    BossDebugPanel::~BossDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void BossDebugPanel::Draw()
    {
        mInspector->Draw("Boss Master");
    }
#else
    void BossDebugPanel::Draw() {}
#endif
}
