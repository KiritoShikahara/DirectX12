#include "apppch.h"
#include "OrbitWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Weapon/OrbitWeaponData.h>

namespace debug
{
    OrbitWeaponDebugPanel::OrbitWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::OrbitWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::OrbitWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    OrbitWeaponDebugPanel::~OrbitWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void OrbitWeaponDebugPanel::Draw()
    {
        mInspector->Draw("Orbit Weapon Master");
    }
#else
    void OrbitWeaponDebugPanel::Draw() {}
#endif
}
