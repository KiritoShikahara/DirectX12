#pragma once

#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>
#include<graphics/Effect/Manager/EffectManager.h>
#include<chrono>
#include<cstdint>
#include<string>
#include<vector>

namespace sys
{
    /// <summary>
    /// フレーム内の主要フェーズごとの所要時間を計測する区分。
    /// ShadowPass/ScenePass/SpritePassは描画コマンド記録用ワーカースレッド上で計測される
    /// </summary>
    enum class ePerfSection : uint8_t
    {
        GameplayUpdate, // ECS System実行)
        Physics,        // 物理ステップ
        RenderCollect,  // 描画データの収集フェーズ。各レンダラーのUpdateAndDraw。
        ShadowPass,     // FbxRenderer::DrawShadowPass(ワーカースレッド)
        ScenePass,      // FbxRenderer::End + SkyboxRenderer::End(ワーカースレッド。主にFBXのDrawCall記録)
        SpritePass,     // Sprite/Shape/Textの描画コマンド記録(ワーカースレッド)
        EffectUpdate,   // Effekseerのパーティクル更新(UpdateGameplay内、EffekseerManager::Update)。
        EffectDraw,     // Effekseerの描画コマンド記録(Render内、EffekseerManager::Draw)。
        Debug,          // デバッグ描画・シーン遷移・ImGuiの描画コマンド記録(メインスレッド)
        Count
    };

    /// <summary>
    /// フレーム時間・FPS・フェーズ別の所要時間の記録をして、GUIに表示する。
    /// </summary>
    class ENGINE_API PerformanceMonitor : public utility::Singleton<PerformanceMonitor>
    {
        SINGLETON_CLASS(PerformanceMonitor);
    public:
        SINGLETON_ACCESSOR(PerformanceMonitor);

        /// <summary>ImGuiデバッグUI(「Performance」ウィンドウ)を登録する</summary>
        void Initialize();

        /// <summary>ImGuiデバッグUIの登録を解除する</summary>
        void Finalize();

        /// <summary>
        /// 1フレーム分のrawDeltaTime(秒)を記録する。
        /// 呼ばれた時点で、直前のフレーム中にBeginSection/EndSectionで蓄積された各区分の
        /// 所要時間を確定させ、次フレーム用にリセットする。
        /// </summary>
        void RecordFrame(float rawDeltaTime);

        /// <summary>指定区分の計測を開始する</summary>
        void BeginSection(ePerfSection section);

        /// <summary>指定区分の計測を終了し、経過時間を当該フレームの合計へ加算する</summary>
        void EndSection(ePerfSection section);

    private:
        void RegisterImgui();

#ifdef ECSE_PERF_TELEMETRY
        /// <summary>
        /// 計測値をCSVへ追記する(ECSE_PERF_TELEMETRY定義時のみ)。ImGuiのPerformanceウィンドウは
        /// _DEBUGビルドにしか存在しないため、Release構成での性能計測手段として用意している。
        /// </summary>
        void DumpTelemetry();
#endif

        // 直近kHistorySizeフレーム分のフレーム時間を保持するリングバッファ
        static constexpr int kHistorySize = 120;
        float mFrameTimeHistoryMs[kHistorySize] = {};
        int   mHistoryCursor = 0; // 次に書き込むインデックス
        int   mHistoryCount = 0;  // 有効なエントリ数(kHistorySizeで頭打ち)

        // 表示用の集計値。毎フレーム更新すると数値が激しく点滅して読みづらいため、
        // 一定間隔(kFpsUpdateIntervalSec)でのみ直近履歴から再計算する
        static constexpr float kFpsUpdateIntervalSec = 0.25f;
        float mFpsUpdateAccumulator = 0.0f;
        float mDisplayedFps = 0.0f;
        float mDisplayedFrameTimeMs = 0.0f;
        float mMinFrameTimeMs = 0.0f;
        float mMaxFrameTimeMs = 0.0f;

        // 1% Low FPS
        // 平均FPSは一瞬のカクつきを均してしまい体感と一致しないため、
        // 描画の安定性を見るにはこちらを主指標にする。
        float mOnePercentLowFps = 0.0f;

        // GPU側の所要時間
        float mDisplayedGpuMs = 0.0f;
        float mGpuAccumMs = 0.0f;
        int   mGpuAccumSamples = 0;

        // 履歴の並べ替え用バッファ
        std::vector<float> mSortedFrameTimes;

        struct SectionState
        {
            std::chrono::high_resolution_clock::time_point StartTime{};
            float CurrentFrameMs = 0.0f; // 今フレーム中にBegin/Endで積算中の値
            float DisplayedMs = 0.0f;    // 表示用に平滑化された値
            float AccumMs = 0.0f;        // 表示更新間隔中の積算値
            int   AccumSamples = 0;
        };
        static constexpr size_t kSectionCount = static_cast<size_t>(ePerfSection::Count);
        SectionState mSections[kSectionCount];

        // RegisterImgui()内、ComponentSystemManagerから集めたSystem別所要時間の一時バッファ。
        struct SystemTimingEntry
        {
            const char* PhaseName = nullptr;
            std::string SystemName;
            float       Ms = 0.0f;
        };
        std::vector<SystemTimingEntry> mSystemTimingEntries;

        // 素材別エフェクト負荷の表示用一時バッファ
        std::vector<graphics::EffekseerManager::EffectStatEntry> mEffectStatEntries;
    };
}
