#include "apppch.h"
#include "EnemyStatusDebugPanel.h"

#include"EnemyStatusComponent.h"
#include<Data/Enemy/EnemyData.h>

namespace debug
{
    EnemyStatusDebugPanel::EnemyStatusDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#ifdef _DEBUG
        auto& mgr = data::DataRegistry::Get().GetManager<data::EnemyData>();
        mInspector = std::make_unique<data::DataInspector<data::EnemyData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    EnemyStatusDebugPanel::~EnemyStatusDebugPanel()
    {
#ifdef _DEBUG
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#ifdef _DEBUG
    void EnemyStatusDebugPanel::Draw()
    {
        // �����e�[�u���G�f�B�^�iLoad/Save CSV�EDB�AAdd Row�AId�܂ޑS�Z���ҏW�APK�d���n�C���C�g�j
        mInspector->Draw("Enemy Master");

        // �ҏW���ʂ𐶑����̓G�֍ēK�p���邽�߂̑���E�B���h�E
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

                st.Recompute();                    // Base �~ WaveMod �� Current
                st.CurrentHp = st.Current.MaxHp;   // HP���^���ɖ߂�
            });
    }
#else
    void EnemyStatusDebugPanel::Draw() {}
    void EnemyStatusDebugPanel::ApplyAllToEnemies() {}
    void EnemyStatusDebugPanel::ApplyRowToEnemies(const data::EnemyData&) {}
#endif
}