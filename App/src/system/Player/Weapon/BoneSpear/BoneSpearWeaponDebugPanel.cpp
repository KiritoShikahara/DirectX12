#include "apppch.h"
#include "BoneSpearWeaponDebugPanel.h"
#include <Utility/config/DebugConfig.h>

#include<Data/Weapon/BoneSpearWeaponData.h>

namespace debug
{
    BoneSpearWeaponDebugPanel::BoneSpearWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::BoneSpearWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::BoneSpearWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    BoneSpearWeaponDebugPanel::~BoneSpearWeaponDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void BoneSpearWeaponDebugPanel::Draw()
    {
        mInspector->Draw("BoneSpear Weapon Master");
    }
#else
    void BoneSpearWeaponDebugPanel::Draw() {}
#endif
}
