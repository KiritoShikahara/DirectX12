#include "apppch.h"
#include "AreaAttackWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/Weapon/AreaAttackWeaponData.h>

namespace debug
{
    AreaAttackWeaponDebugPanel::AreaAttackWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::AreaAttackWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::AreaAttackWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    AreaAttackWeaponDebugPanel::~AreaAttackWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void AreaAttackWeaponDebugPanel::Draw()
    {
        mInspector->Draw("AreaAttack Weapon Master");
    }
#else
    void AreaAttackWeaponDebugPanel::Draw() {}
#endif
}
