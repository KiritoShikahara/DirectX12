#include "pch.h"
#include "PerformanceMonitor.h"

#include<ecs/system/manager/ComponentSystemManager.h>
#include<algorithm>

namespace
{
    constexpr const char* kSectionNames[] =
    {
        "Gameplay Update",
        "Physics",
        "Shadow Pass",
        "Scene Pass (FBX)",
        "Sprite Pass",
        "Effect",
        "Debug/ImGui",
    };
    static_assert(sizeof(kSectionNames) / sizeof(kSectionNames[0])
        == static_cast<size_t>(sys::ePerfSection::Count), "kSectionNames does not match ePerfSection");
}

namespace sys
{
    void PerformanceMonitor::Initialize()
    {
#if defined(_DEBUG) || defined(DEV_TOOL_ENABLED)
        ImGuiManager::Get().AddDebugUI([this]()
            {
                this->RegisterImgui();
            }, "Performance");
#endif
    }

    void PerformanceMonitor::Finalize()
    {
#if defined(_DEBUG) || defined(DEV_TOOL_ENABLED)
        ImGuiManager::Get().RemoveDebugUI("Performance");
#endif
    }

    void PerformanceMonitor::RecordFrame(float rawDeltaTime)
    {
        const float frameTimeMs = rawDeltaTime * 1000.0f;

        mFrameTimeHistoryMs[mHistoryCursor] = frameTimeMs;
        mHistoryCursor = (mHistoryCursor + 1) % kHistorySize;
        mHistoryCount = std::min(mHistoryCount + 1, kHistorySize);

        // 直前のフレーム中にBeginSection/EndSectionで積算された各区分の所要時間を
        // 平均化用バッファへ積み、今フレーム用にリセットする(ワーカースレッドが書き込んだ値は
        // 直前フレームのRender()末尾のWaitAll()で同期済みのため、ここで安全に読める)
        for (auto& s : mSections)
        {
            s.AccumMs += s.CurrentFrameMs;
            s.AccumSamples += 1;
            s.CurrentFrameMs = 0.0f;
        }

        // 表示用の数値は0.25秒ごとにのみ更新する(毎フレーム更新すると数値が
        // 激しく点滅して読みづらいため)。表示値は直近履歴の平均・最小・最大から求める。
        mFpsUpdateAccumulator += rawDeltaTime;
        if (mFpsUpdateAccumulator < kFpsUpdateIntervalSec || mHistoryCount == 0)
        {
            return;
        }
        mFpsUpdateAccumulator = 0.0f;

        float sum = 0.0f;
        float minMs = mFrameTimeHistoryMs[0];
        float maxMs = mFrameTimeHistoryMs[0];
        for (int i = 0; i < mHistoryCount; ++i)
        {
            const float v = mFrameTimeHistoryMs[i];
            sum += v;
            minMs = std::min(minMs, v);
            maxMs = std::max(maxMs, v);
        }

        mDisplayedFrameTimeMs = sum / static_cast<float>(mHistoryCount);
        mDisplayedFps = (mDisplayedFrameTimeMs > 0.0f) ? (1000.0f / mDisplayedFrameTimeMs) : 0.0f;
        mMinFrameTimeMs = minMs;
        mMaxFrameTimeMs = maxMs;

        for (auto& s : mSections)
        {
            if (s.AccumSamples > 0)
            {
                s.DisplayedMs = s.AccumMs / static_cast<float>(s.AccumSamples);
            }
            s.AccumMs = 0.0f;
            s.AccumSamples = 0;
        }
    }

    void PerformanceMonitor::BeginSection(ePerfSection section)
    {
        mSections[static_cast<size_t>(section)].StartTime = std::chrono::high_resolution_clock::now();
    }

    void PerformanceMonitor::EndSection(ePerfSection section)
    {
        auto& s = mSections[static_cast<size_t>(section)];
        const auto now = std::chrono::high_resolution_clock::now();
        const float ms = std::chrono::duration<float, std::milli>(now - s.StartTime).count();
        s.CurrentFrameMs += ms;
    }

    void PerformanceMonitor::RegisterImgui()
    {
        ImGui::SetNextWindowSize(ImVec2(360, 340), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Performance"))
        {
            ImGui::End();
            return;
        }

        // FPSが低いほど赤く、高いほど緑になる簡易な色分け(60FPS以上=緑、30FPS以下=赤)
        const float t = std::clamp((mDisplayedFps - 30.0f) / 30.0f, 0.0f, 1.0f);
        const ImVec4 fpsColor = { 1.0f - t, t, 0.0f, 1.0f };

        ImGui::TextColored(fpsColor, "FPS: %.1f", mDisplayedFps);
        ImGui::Text("Frame Time: %.2f ms  (min %.2f / max %.2f)",
            mDisplayedFrameTimeMs, mMinFrameTimeMs, mMaxFrameTimeMs);
        ImGui::Separator();

        // 直近kHistorySizeフレームのフレーム時間推移(ミリ秒)を時系列順(古い→新しい)に並べ直す。
        // 急なドロップ(スパイク)を目視で確認できるようにする
        float ordered[kHistorySize];
        const int start = (mHistoryCursor - mHistoryCount + kHistorySize) % kHistorySize;
        for (int i = 0; i < mHistoryCount; ++i)
        {
            ordered[i] = mFrameTimeHistoryMs[(start + i) % kHistorySize];
        }

        char overlay[32];
        snprintf(overlay, sizeof(overlay), "%.1f ms", mDisplayedFrameTimeMs);

        ImGui::PlotLines(
            "##FrameTime",
            ordered,
            mHistoryCount,
            0,
            overlay,
            0.0f,
            33.3f, // 30FPS相当を上限目安として表示(それ以上はグラフ天井に張り付く形で視認できる)
            ImVec2(0, 80));

        ImGui::Separator();
        ImGui::Text("Breakdown (avg ms/frame):");
        // Shadow/Scene/SpriteはEffect/Debugと並行実行されるワーカースレッド区分のため、
        // 単純合計はフレーム時間と一致しない(重なりがある)点に注意
        for (size_t i = 0; i < kSectionCount; ++i)
        {
            ImGui::Text("  %-18s %6.2f ms", kSectionNames[i], mSections[i].DisplayedMs);
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("System Timings (top 10)"))
        {
            // Gameplay Update内(PreUpdate/Update/PostUpdate)に登録されている各Systemの
            // 所要時間をComponentSystemManagerから集め、重い順に上位のみ表示する
            mSystemTimingEntries.clear();

            static const ecs::eUpdatePhase kPhases[] =
            {
                ecs::eUpdatePhase::PreUpdate,
                ecs::eUpdatePhase::Update,
                ecs::eUpdatePhase::PostUpdate,
            };
            static const char* kPhaseNames[] = { "PreUpdate", "Update", "PostUpdate" };

            auto& sysMgr = ecs::ComponentSystemManager::Get();
            for (int p = 0; p < 3; ++p)
            {
                for (const auto& t : sysMgr.GetSystemTimings(kPhases[p]))
                {
                    mSystemTimingEntries.push_back({ kPhaseNames[p], t.Name, t.SmoothedMs });
                }
            }

            std::sort(mSystemTimingEntries.begin(), mSystemTimingEntries.end(),
                [](const SystemTimingEntry& a, const SystemTimingEntry& b) { return a.Ms > b.Ms; });

            if (mSystemTimingEntries.empty())
            {
                ImGui::TextDisabled("  (no data yet)");
            }

            const size_t showCount = std::min<size_t>(10, mSystemTimingEntries.size());
            for (size_t i = 0; i < showCount; ++i)
            {
                const auto& e = mSystemTimingEntries[i];
                ImGui::Text("  [%-10s] %-42s %6.3f ms", e.PhaseName, e.SystemName.c_str(), e.Ms);
            }
        }

        ImGui::End();
    }
}
