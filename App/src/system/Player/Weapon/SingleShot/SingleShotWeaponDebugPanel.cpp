#include "apppch.h"
#include "SingleShotWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/Weapon/SingleShotWeaponData.h>

namespace debug
{
    SingleShotWeaponDebugPanel::SingleShotWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::SingleShotWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::SingleShotWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    SingleShotWeaponDebugPanel::~SingleShotWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void SingleShotWeaponDebugPanel::Draw()
    {
        mInspector->Draw("SingleShot Weapon Master");
    }
#else
    void SingleShotWeaponDebugPanel::Draw() {}
#endif
}
