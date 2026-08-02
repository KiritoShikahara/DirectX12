#include "apppch.h"
#include "FlickerStrikeWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/Weapon/FlickerStrikeWeaponData.h>

namespace debug
{
    FlickerStrikeWeaponDebugPanel::FlickerStrikeWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::FlickerStrikeWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::FlickerStrikeWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    FlickerStrikeWeaponDebugPanel::~FlickerStrikeWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void FlickerStrikeWeaponDebugPanel::Draw()
    {
        mInspector->Draw("FlickerStrike Weapon Master");
    }
#else
    void FlickerStrikeWeaponDebugPanel::Draw() {}
#endif
}
