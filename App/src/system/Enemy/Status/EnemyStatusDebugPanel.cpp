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
        // 既存テーブルエディタ（Load/Save CSV・DB、Add Row、Id含む全セル編集、PK重複ハイライト）
        mInspector->Draw("Enemy Master");

        // 編集結果を生存中の敵へ再適用するための操作ウィンドウ
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

                st.Recompute();                    // Base × WaveMod → Current
                st.CurrentHp = st.Current.MaxHp;   // HP満タンに戻す
            });
    }
#else
    void EnemyStatusDebugPanel::Draw() {}
    void EnemyStatusDebugPanel::ApplyAllToEnemies() {}
    void EnemyStatusDebugPanel::ApplyRowToEnemies(const data::EnemyData&) {}
#endif
}