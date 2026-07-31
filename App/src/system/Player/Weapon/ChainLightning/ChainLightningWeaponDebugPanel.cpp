#include "apppch.h"
#include "ChainLightningWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Weapon/ChainLightningWeaponData.h>

namespace debug
{
    ChainLightningWeaponDebugPanel::ChainLightningWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::ChainLightningWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::ChainLightningWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    ChainLightningWeaponDebugPanel::~ChainLightningWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void ChainLightningWeaponDebugPanel::Draw()
    {
        mInspector->Draw("ChainLightning Weapon Master");
    }
#else
    void ChainLightningWeaponDebugPanel::Draw() {}
#endif
}
