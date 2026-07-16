#include "apppch.h"
#include "UltimateDebugPanel.h"

#include<Data/Ultimate/UltimateData.h>

namespace debug
{
    UltimateDebugPanel::UltimateDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#ifdef _DEBUG
        auto& mgr = data::DataRegistry::Get().GetManager<data::UltimateData>();
        mInspector = std::make_unique<data::DataInspector<data::UltimateData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    UltimateDebugPanel::~UltimateDebugPanel()
    {
#ifdef _DEBUG
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#ifdef _DEBUG
    void UltimateDebugPanel::Draw()
    {
        mInspector->Draw("Ultimate Master");
    }
#else
    void UltimateDebugPanel::Draw() {}
#endif
}
