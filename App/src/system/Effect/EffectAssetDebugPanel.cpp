#include "apppch.h"
#include "EffectAssetDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/Effect/EffectAssetData.h>

namespace debug
{
    EffectAssetDebugPanel::EffectAssetDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::EffectAssetData>();
        mInspector = std::make_unique<data::DataInspector<data::EffectAssetData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    EffectAssetDebugPanel::~EffectAssetDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void EffectAssetDebugPanel::Draw()
    {
        mInspector->Draw("EffectAsset Master");
    }
#else
    void EffectAssetDebugPanel::Draw() {}
#endif
}
