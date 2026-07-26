#include "apppch.h"
#include "WaveDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include"WaveComponent.h"
#include<Data/Wave/WaveData.h>

namespace debug
{
    WaveDebugPanel::WaveDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& mgr = data::DataRegistry::Get().GetManager<data::WaveData>();
        mInspector = std::make_unique<data::DataInspector<data::WaveData>>(mgr);

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    WaveDebugPanel::~WaveDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
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
                wave.SpawnCountPerTick = row->SpawnCountPerTick;
                wave.SpawnMarginMin = row->SpawnMarginMin;
                wave.SpawnMarginMax = row->SpawnMarginMax;
                wave.MaxAliveEnemy = row->MaxAliveEnemy;
                wave.StatGrowthStepInterval = row->StatGrowthStepInterval;
                wave.StatGrowthPerStep = row->StatGrowthPerStep;
                wave.MiniBossFirstSpawnTime = row->MiniBossFirstSpawnTime;
                wave.MiniBossInterval = row->MiniBossInterval;
                wave.MidBossSpawnTime = row->MidBossSpawnTime;
                wave.FinalBossSpawnTime = row->FinalBossSpawnTime;
                wave.ClearTime = row->ClearTime;
            });
    }
#else
    void WaveDebugPanel::Draw() {}
    void WaveDebugPanel::ApplyToRunningWave() {}
#endif
}
