#include "apppch.h"
#include "SingleShotWeaponDebugPanel.h"

#include<Data/Weapon/SingleShotWeaponData.h>

namespace debug
{
    SingleShotWeaponDebugPanel::SingleShotWeaponDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#ifdef _DEBUG
        auto& mgr = data::DataRegistry::Get().GetManager<data::SingleShotWeaponData>();
        mInspector = std::make_unique<data::DataInspector<data::SingleShotWeaponData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    SingleShotWeaponDebugPanel::~SingleShotWeaponDebugPanel()
    {
#ifdef _DEBUG
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#ifdef _DEBUG
    void SingleShotWeaponDebugPanel::Draw()
    {
        mInspector->Draw("SingleShot Weapon Master");
    }
#else
    void SingleShotWeaponDebugPanel::Draw() {}
#endif
}
