#include "pch.h"
#include "PerformanceMonitor.h"

#include<ecs/system/manager/ComponentSystemManager.h>
#include<graphics/Effect/Manager/EffectManager.h>
#include<graphics/Profiler/GpuProfiler.h>
#include<algorithm>
#include<functional>

namespace
{
    constexpr const char* kSectionNames[] =
    {
        "Gameplay Update",
        "Physics",
        "Render Collect",
        "Shadow Pass",
        "Scene Pass (FBX)",
        "Sprite Pass",
        "Effect Update",
        "Effect Draw",
        "Debug/ImGui",
    };
    static_assert(sizeof(kSectionNames) / sizeof(kSectionNames[0])
        == static_cast<size_t>(sys::ePerfSection::Count), "kSectionNames does not match ePerfSection");
}

namespace sys
{
    void PerformanceMonitor::Initialize()
    {
#if DEV_TOOL_ENABLED
        ImGuiManager::Get().AddDebugUI([this]()
            {
                this->RegisterImgui();
            }, "Performance");
#endif
    }

    void PerformanceMonitor::Finalize()
    {
#if DEV_TOOL_ENABLED
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

        // GPU時間はGpuProfilerがFRAME_COUNTフレーム遅れで回収するため、
        // ここで毎フレーム拾って平均化用に積む
        if (graphics::GpuProfiler::Get().IsAvailable())
        {
            mGpuAccumMs += graphics::GpuProfiler::Get().GetFrameMs();
            mGpuAccumSamples += 1;
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

        // 1% Low: 遅い方から1%(最低1フレーム)の平均フレーム時間から求める。
        // 平均FPSはカクつきを均してしまうため、体感の滑らかさはこちらに現れる
        mSortedFrameTimes.assign(mFrameTimeHistoryMs, mFrameTimeHistoryMs + mHistoryCount);
        std::sort(mSortedFrameTimes.begin(), mSortedFrameTimes.end(), std::greater<float>());

        const int lowCount = std::max(1, mHistoryCount / 100);
        float lowSum = 0.0f;
        for (int i = 0; i < lowCount; ++i) lowSum += mSortedFrameTimes[i];
        const float lowAvgMs = lowSum / static_cast<float>(lowCount);
        mOnePercentLowFps = (lowAvgMs > 0.0f) ? (1000.0f / lowAvgMs) : 0.0f;

        // GPU時間も同じ間隔で平均化する
        mDisplayedGpuMs = (mGpuAccumSamples > 0)
            ? (mGpuAccumMs / static_cast<float>(mGpuAccumSamples))
            : 0.0f;
        mGpuAccumMs = 0.0f;
        mGpuAccumSamples = 0;

        for (auto& s : mSections)
        {
            if (s.AccumSamples > 0)
            {
                s.DisplayedMs = s.AccumMs / static_cast<float>(s.AccumSamples);
            }
            s.AccumMs = 0.0f;
            s.AccumSamples = 0;
        }

#ifdef ECSE_PERF_TELEMETRY
        DumpTelemetry();
#endif
    }

#ifdef ECSE_PERF_TELEMETRY
    void PerformanceMonitor::DumpTelemetry()
    {
        // ImGuiのPerformanceウィンドウは_DEBUGビルドにしか存在しないため、Release構成での
        // 計測手段としてCSVへ追記する。ECSE_PERF_TELEMETRY定義時のみ有効な計測用の仕組み。
        static FILE* file = nullptr;
        if (file == nullptr)
        {
            fopen_s(&file, "perf_telemetry.csv", "w");
            if (file == nullptr) return;
            fprintf(file, "fps,low1pct,cpu_ms,gpu_ms");
            for (size_t i = 0; i < kSectionCount; ++i) fprintf(file, ",%s", kSectionNames[i]);
            fprintf(file, ",gpu_shadow,gpu_scene,gpu_effect,gpu_sprite");
            fprintf(file, ",effect_calls,effect_verts,effect_instances\n");
        }

        auto& effect = graphics::EffekseerManager::Get();
        auto& gpu = graphics::GpuProfiler::Get();
        fprintf(file, "%.1f,%.1f,%.2f,%.2f",
            mDisplayedFps, mOnePercentLowFps, mDisplayedFrameTimeMs, mDisplayedGpuMs);
        for (size_t i = 0; i < kSectionCount; ++i) fprintf(file, ",%.2f", mSections[i].DisplayedMs);
        fprintf(file, ",%.3f,%.3f,%.3f,%.3f",
            gpu.GetChannelMs(graphics::eRenderChannel::Shadow),
            gpu.GetChannelMs(graphics::eRenderChannel::Scene),
            gpu.GetChannelMs(graphics::eRenderChannel::Effect),
            gpu.GetChannelMs(graphics::eRenderChannel::Sprite));
        fprintf(file, ",%d,%d,%d\n",
            effect.GetLastDrawCallCount(),
            effect.GetLastDrawVertexCount(),
            effect.GetLastInstanceCount());
        fflush(file);
    }
#endif

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
        ImGui::SameLine();
        ImGui::TextDisabled("| 1%% Low: %.1f", mOnePercentLowFps);

        ImGui::Text("Frame Time: %.2f ms  (min %.2f / max %.2f)",
            mDisplayedFrameTimeMs, mMinFrameTimeMs, mMaxFrameTimeMs);

        // CPU(コマンド記録)とGPU(実行)を並べて表示する。
        // どちらが律速かでとるべき対策が全く変わるため、両方を常に見えるようにしておく
        // (GPU時間は計測非対応の環境ではn/aと表示する)
        auto& gpu = graphics::GpuProfiler::Get();
        if (gpu.IsAvailable())
        {
            const bool gpuBound = (mDisplayedGpuMs > mDisplayedFrameTimeMs * 0.9f);
            ImGui::Text("CPU: %.2f ms", mDisplayedFrameTimeMs);
            ImGui::SameLine();
            ImGui::TextColored(
                gpuBound ? ImVec4(1.0f, 0.6f, 0.2f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                "| GPU: %.2f ms%s", mDisplayedGpuMs, gpuBound ? "  <- GPU bound" : "");
        }
        else
        {
            ImGui::Text("CPU: %.2f ms", mDisplayedFrameTimeMs);
            ImGui::SameLine();
            ImGui::TextDisabled("| GPU: n/a");
        }

        // V-Sync有効時は表示リフレッシュレートで頭打ちになるため、
        // 「これ以上速くならない」状態と「処理が重い」状態を取り違えないよう明示する
        if (mDisplayedFrameTimeMs > 0.0f && mDisplayedGpuMs > 0.0f)
        {
            const float busiestMs = std::max(mDisplayedGpuMs, mDisplayedFrameTimeMs);
            if (busiestMs < mDisplayedFrameTimeMs * 0.5f)
            {
                ImGui::TextDisabled("(waiting on V-Sync: plenty of headroom)");
            }
        }

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

        // Effect Drawの負荷はEffekseer/LLGI内部の1呼び出しあたり固定コスト×呼び出し回数に
        // ほぼ比例するため、msの数値だけでなく実際の呼び出し回数も並べて表示する
        // (EffectManager::Draw()参照。ms値が高い時にコンテンツ側の削減余地があるかの判断材料)
        auto& effect = graphics::EffekseerManager::Get();
        ImGui::Text("  %-18s calls=%-5d verts=%-6d instances=%-5d",
            "Effect Draw Stat",
            effect.GetLastDrawCallCount(),
            effect.GetLastDrawVertexCount(),
            effect.GetLastInstanceCount());

        ImGui::Separator();
        if (ImGui::CollapsingHeader("GPU Breakdown (by render channel)"))
        {
            // CPU側の内訳(上のBreakdown)は「コマンドを積むのに要した時間」であり、
            // GPUが実際に描くのに要した時間はこちらにしか現れない
            auto& gpuProfiler = graphics::GpuProfiler::Get();
            if (!gpuProfiler.IsAvailable())
            {
                ImGui::TextDisabled("  (timestamp queries unavailable)");
            }
            else
            {
                static const char* kChannelNames[] =
                {
                    "Pre (clear)", "Shadow", "Scene (FBX)", "Effect", "Sprite", "Debug", "Post"
                };
                static_assert(sizeof(kChannelNames) / sizeof(kChannelNames[0]) == graphics::CHANNEL_COUNT,
                    "kChannelNames does not match eRenderChannel");

                for (uint32_t i = 0; i < graphics::CHANNEL_COUNT; ++i)
                {
                    ImGui::Text("  %-14s %6.3f ms", kChannelNames[i],
                        gpuProfiler.GetChannelMs(static_cast<graphics::eRenderChannel>(i)));
                }
                ImGui::Separator();
                ImGui::Text("  %-14s %6.3f ms", "GPU total", gpuProfiler.GetFrameMs());
            }
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Effect Breakdown (by asset)"))
        {
            // Effect Drawの負荷はパーティクル(インスタンス)数にほぼ比例するため、
            // 素材別のインスタンス数が「どの.efkを削れば効くか」を直接示す
            mEffectStatEntries.clear();
            for (const auto& s : graphics::EffekseerManager::Get().GetEffectStats())
            {
                mEffectStatEntries.push_back(s);
            }

            std::sort(mEffectStatEntries.begin(), mEffectStatEntries.end(),
                [](const auto& a, const auto& b) { return a.InstanceCount > b.InstanceCount; });

            if (mEffectStatEntries.empty())
            {
                ImGui::TextDisabled("  (no active effects)");
            }

            int32_t totalInstances = 0;
            for (const auto& e : mEffectStatEntries) totalInstances += e.InstanceCount;

            for (const auto& e : mEffectStatEntries)
            {
                const float percent = (totalInstances > 0)
                    ? (100.0f * static_cast<float>(e.InstanceCount) / static_cast<float>(totalInstances))
                    : 0.0f;
                ImGui::Text("  %-28s inst=%-6d (%4.1f%%)  handles=%d",
                    e.Name != nullptr ? e.Name->c_str() : "(unknown)",
                    e.InstanceCount, percent, e.HandleCount);
            }
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
