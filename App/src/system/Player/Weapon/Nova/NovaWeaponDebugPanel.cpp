#include "apppch.h"
#include "NovaWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Weapon/NovaWeaponData.h>

namespace debug
{
    NovaWeaponDebugPanel::NovaWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::NovaWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::NovaWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    NovaWeaponDebugPanel::~NovaWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void NovaWeaponDebugPanel::Draw()
    {
        mInspector->Draw("Nova Weapon Master");
    }
#else
    void NovaWeaponDebugPanel::Draw() {}
#endif
}
