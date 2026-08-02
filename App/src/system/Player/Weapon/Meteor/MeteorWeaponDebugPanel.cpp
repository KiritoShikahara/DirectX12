#include "apppch.h"
#include "MeteorWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/Weapon/MeteorWeaponData.h>

namespace debug
{
    MeteorWeaponDebugPanel::MeteorWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::MeteorWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::MeteorWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    MeteorWeaponDebugPanel::~MeteorWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void MeteorWeaponDebugPanel::Draw()
    {
        mInspector->Draw("Meteor Weapon Master");
    }
#else
    void MeteorWeaponDebugPanel::Draw() {}
#endif
}
