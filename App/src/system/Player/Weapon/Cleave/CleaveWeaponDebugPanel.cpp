#include "apppch.h"
#include "CleaveWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Weapon/CleaveWeaponData.h>

namespace debug
{
    CleaveWeaponDebugPanel::CleaveWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::CleaveWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::CleaveWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    CleaveWeaponDebugPanel::~CleaveWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void CleaveWeaponDebugPanel::Draw()
    {
        mInspector->Draw("Cleave Weapon Master");
    }
#else
    void CleaveWeaponDebugPanel::Draw() {}
#endif
}
