#include "apppch.h"
#include "UltimateDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Ultimate/UltimateData.h>

namespace debug
{
    UltimateDebugPanel::UltimateDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::UltimateData>();
        mInspector = std::make_unique<data::DataInspector<data::UltimateData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    UltimateDebugPanel::~UltimateDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void UltimateDebugPanel::Draw()
    {
        mInspector->Draw("Ultimate Master");
    }
#else
    void UltimateDebugPanel::Draw() {}
#endif
}
