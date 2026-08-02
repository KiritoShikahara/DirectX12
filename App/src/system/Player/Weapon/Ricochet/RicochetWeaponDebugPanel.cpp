#include "apppch.h"
#include "RicochetWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/Weapon/RicochetWeaponData.h>

namespace debug
{
    RicochetWeaponDebugPanel::RicochetWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::RicochetWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::RicochetWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    RicochetWeaponDebugPanel::~RicochetWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void RicochetWeaponDebugPanel::Draw()
    {
        mInspector->Draw("Ricochet Weapon Master");
    }
#else
    void RicochetWeaponDebugPanel::Draw() {}
#endif
}
