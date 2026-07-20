#include "apppch.h"
#include "PerkDebugPanel.h"

#include <Data/Perk/PerkData.h>
#include <Utility/config/DebugConfig.h>

namespace debug
{
    PerkDebugPanel::PerkDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::PerkData>();
        mInspector = std::make_unique<data::DataInspector<data::PerkData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    PerkDebugPanel::~PerkDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void PerkDebugPanel::Draw()
    {
        // Weightの意味が分からないと調整できないため、テーブルの前に説明を出す
        if (ImGui::Begin("Perk Weight Help"))
        {
            ImGui::TextUnformatted("Weight = relative draw rate for the");
            ImGui::TextUnformatted("'other' slots (choice 3-5).");
            ImGui::TextUnformatted("Higher = more likely. 0 = never appears.");
            ImGui::Separator();
            ImGui::TextUnformatted("Choice 1 = new weapon (or level up)");
            ImGui::TextUnformatted("Choice 2 = weapon level up");
            ImGui::TextUnformatted("These two ignore Weight.");
            ImGui::Separator();
            ImGui::TextUnformatted("Edit below, then Save to CSV/DB.");
            ImGui::TextDisabled("Changes apply to the next perk selection.");
        }
        ImGui::End();

        // 唯一のテーブルエディタ（Load/Save CSV・DB、Id含む全セル編集）
        mInspector->Draw("Perk Master");
    }
#else
    void PerkDebugPanel::Draw() {}
#endif
}
