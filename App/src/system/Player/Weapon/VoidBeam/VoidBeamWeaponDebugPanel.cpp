#include "apppch.h"
#include "VoidBeamWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/Weapon/VoidBeamWeaponData.h>

namespace debug
{
    VoidBeamWeaponDebugPanel::VoidBeamWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::VoidBeamWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::VoidBeamWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    VoidBeamWeaponDebugPanel::~VoidBeamWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void VoidBeamWeaponDebugPanel::Draw()
    {
        mInspector->Draw("VoidBeam Weapon Master");
    }
#else
    void VoidBeamWeaponDebugPanel::Draw() {}
#endif
}
