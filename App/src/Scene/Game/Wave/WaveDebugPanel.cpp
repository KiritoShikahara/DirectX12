#include "apppch.h"
#include "WaveDebugPanel.h"

#include"WaveComponent.h"
#include<Data/Wave/WaveData.h>

namespace debug
{
    WaveDebugPanel::WaveDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#ifdef _DEBUG
        auto& mgr = data::DataRegistry::Get().GetManager<data::WaveData>();
        mInspector = std::make_unique<data::DataInspector<data::WaveData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    WaveDebugPanel::~WaveDebugPanel()
    {
#ifdef _DEBUG
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#ifdef _DEBUG
    void WaveDebugPanel::Draw()
    {
        // 唯一のテーブルエディタ（Load/Save CSV・DB、Id含む全セル編集）
        mInspector->Draw("Wave Master");

        // 編集結果を実行中のWaveComponentへ再適用するための操作ウィンドウ
        // （WaveComponentはCreateStateController実行時に一度だけ初期化されるため、
        // プレイ中に調整した数値を試すにはこのボタンでの再適用が必要）
        if (ImGui::Begin("Wave Apply"))
        {
            if (ImGui::Button("Apply to Running Wave"))
            {
                ApplyToRunningWave();
            }
        }
        ImGui::End();
    }

    void WaveDebugPanel::ApplyToRunningWave()
    {
        const auto* row = data::DataRegistry::Get().GetManager<data::WaveData>().GetById(0);
        if (row == nullptr) return;

        auto& registry = ecs::EntityManager::Get().GetRegistry();
        registry.view<::ecs::WaveComponent>().each(
            [&](::ecs::WaveComponent& wave)
            {
                wave.SpawnInterval = row->SpawnInterval;
                wave.SpawnMarginMin = row->SpawnMarginMin;
                wave.SpawnMarginMax = row->SpawnMarginMax;
                wave.StatGrowthPerSecond = row->StatGrowthPerSecond;
                wave.BossSpawnTime = row->BossSpawnTime;
                wave.ClearTime = row->ClearTime;
            });
    }
#else
    void WaveDebugPanel::Draw() {}
    void WaveDebugPanel::ApplyToRunningWave() {}
#endif
}
