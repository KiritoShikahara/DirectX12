#include "apppch.h"
#include "EnemyStatusDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop荳｡譁ｹ縺ｧ譛牙柑)繧貞盾辣ｧ縺吶ｋ縺溘ａ逶ｴ謗･include

#include"EnemyStatusComponent.h"
#include<Data/Enemy/EnemyData.h>

namespace debug
{
    EnemyStatusDebugPanel::EnemyStatusDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::EnemyData>();
        mInspector = std::make_unique<data::DataInspector<data::EnemyData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    EnemyStatusDebugPanel::~EnemyStatusDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void EnemyStatusDebugPanel::Draw()
    {
        // ・ｽ・ｽ・ｽ・ｽ・ｽe・ｽ[・ｽu・ｽ・ｽ・ｽG・ｽf・ｽB・ｽ^・ｽiLoad/Save CSV・ｽEDB・ｽAAdd Row・ｽAId・ｽﾜむ全・ｽZ・ｽ・ｽ・ｽﾒ集・ｽAPK・ｽd・ｽ・ｽ・ｽn・ｽC・ｽ・ｽ・ｽC・ｽg・ｽj
        mInspector->Draw("Enemy Master");

        // ・ｽﾒ集・ｽ・ｽ・ｽﾊを生托ｿｽ・ｽ・ｽ・ｽﾌ敵・ｽﾖ再適・ｽp・ｽ・ｽ・ｽ驍ｽ・ｽﾟの托ｿｽ・ｽ・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE
        if (ImGui::Begin("Enemy Apply"))
        {

            if (ImGui::Button("Apply All to Enemies"))
            {
                ApplyAllToEnemies();
            }
        }
        ImGui::End();
    }

    void EnemyStatusDebugPanel::ApplyAllToEnemies()
    {
        auto& mgr = data::DataRegistry::Get().GetManager<data::EnemyData>();
        for (const auto& row : mgr.GetAll())
        {
            ApplyRowToEnemies(row);
        }
    }

    void EnemyStatusDebugPanel::ApplyRowToEnemies(const data::EnemyData& row)
    {
        auto& registry = ecs::EntityManager::Get().GetRegistry();

        registry.view<ecs::EnemyStatusComponent>().each(
            [&](ecs::EnemyStatusComponent& st)
            {
                if (st.EnemyId != row.Id) return;

                st.Base.MaxHp = row.MaxHp;
                st.Base.MoveSpeed = row.MoveSpeed;
                st.Base.AtkPower = row.AtkPower;
                st.Base.ExperienceValue = static_cast<float>(row.Exp);
                st.Base.GoldValue = row.GoldValue;

                st.Recompute();                    // Base ・ｽ~ WaveMod ・ｽ・ｽ Current
                st.CurrentHp = st.Current.MaxHp;   // HP・ｽ・ｽ・ｽ^・ｽ・ｽ・ｽﾉ戻ゑｿｽ
            });
    }
#else
    void EnemyStatusDebugPanel::Draw() {}
    void EnemyStatusDebugPanel::ApplyAllToEnemies() {}
    void EnemyStatusDebugPanel::ApplyRowToEnemies(const data::EnemyData&) {}
#endif
}