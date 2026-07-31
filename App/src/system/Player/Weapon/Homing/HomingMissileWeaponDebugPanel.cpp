#include "apppch.h"
#include "HomingMissileWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Weapon/HomingMissileWeaponData.h>

namespace debug
{
    HomingMissileWeaponDebugPanel::HomingMissileWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::HomingMissileWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::HomingMissileWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    HomingMissileWeaponDebugPanel::~HomingMissileWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void HomingMissileWeaponDebugPanel::Draw()
    {
        mInspector->Draw("HomingMissile Weapon Master");
    }
#else
    void HomingMissileWeaponDebugPanel::Draw() {}
#endif
}
