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
    /// (それぞれ専用スロットに書き込むため、他区分と競合しない)。
    /// </summary>
    enum class ePerfSection : uint8_t
    {
        GameplayUpdate, // ECS System実行(PreUpdate+Update+PostUpdate、物理を除く)
        Physics,        // 物理ステップ(BuildPendingBodies〜PhysicsSystem::Update〜SyncToTransform)
        RenderCollect,  // 描画データの収集フェーズ(メインスレッド)。各レンダラーのUpdateAndDraw。
                        // FBXのボーン行列計算(CalcBoneMatrices)もここに含まれるため、
                        // キャラクター数が多い場合はここが支配的になりうる
        ShadowPass,     // FbxRenderer::DrawShadowPass(ワーカースレッド)
        ScenePass,      // FbxRenderer::End + SkyboxRenderer::End(ワーカースレッド。主にFBXのDrawCall記録)
        SpritePass,     // Sprite/Shape/Textの描画コマンド記録(ワーカースレッド)
        EffectUpdate,   // Effekseerのパーティクル更新(UpdateGameplay内、EffekseerManager::Update)。
                        // 現状LaunchWorkerThreads()を呼んでいないため完全にシングルスレッドで実行される
                        // (EffekseerManager::Initialize()のコメント参照)
        EffectDraw,     // Effekseerの描画コマンド記録(Render内、EffekseerManager::Draw)。
                        // Effekseerはスレッドセーフでないため他レンダラーのワーカースレッドとは
                        // 並行実行されず、メインスレッドで専用チャネルとして記録される
        Debug,          // デバッグ描画・シーン遷移・ImGuiの描画コマンド記録(メインスレッド)
        Count
    };

    /// <summary>
    /// フレーム時間・FPS・フェーズ別所要時間を記録し、ImGuiデバッグUI(「Performance」ウィンドウ)へ
    /// 表示する。Engine::Update()から毎フレームRecordFrame()を呼ぶことでフレーム時間履歴を蓄積し、
    /// Engine側の各フェーズをBeginSection()/EndSection()で挟むことで内訳を計測する。
    /// 数値表示はTimeScaleの影響を受けない生のフレーム時間(rawDeltaTime)を元にする
    /// (スローモーション演出中もFPSが正しく見えるようにするため)。
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
        /// 1フレーム分のrawDeltaTime(秒)を記録する。Engine::Update()の先頭から毎フレーム呼ぶこと。
        /// 呼ばれた時点で、直前のフレーム中にBeginSection/EndSectionで蓄積された各区分の
        /// 所要時間を確定させ、次フレーム用にリセットする。
        /// </summary>
        void RecordFrame(float rawDeltaTime);

        /// <summary>指定区分の計測を開始する(同区分のBegin/Endはネストしないこと)</summary>
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

        // 直近kHistorySizeフレーム分のフレーム時間(ミリ秒)を保持するリングバッファ
        // (グラフ表示用。約2秒分@60FPS)
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

        // 1% Low FPS(遅い方から1%のフレームの平均から求めるFPS)。
        // 平均FPSは一瞬のカクつきを均してしまい体感と一致しないため、
        // 描画の安定性を見るにはこちらを主指標にする。
        float mOnePercentLowFps = 0.0f;

        // GPU側の所要時間(GpuProfilerから取得。CPUと同じ間隔で平滑化する)
        float mDisplayedGpuMs = 0.0f;
        float mGpuAccumMs = 0.0f;
        int   mGpuAccumSamples = 0;

        // 履歴の並べ替え用バッファ(1% Lowの算出に使う。毎フレームのvector生成を避けて使い回す)
        std::vector<float> mSortedFrameTimes;

        struct SectionState
        {
            std::chrono::high_resolution_clock::time_point StartTime{};
            float CurrentFrameMs = 0.0f; // 今フレーム中にBegin/Endで積算中の値
            float DisplayedMs = 0.0f;    // 表示用に平滑化された値(FPSと同じ間隔で更新)
            float AccumMs = 0.0f;        // 表示更新間隔中の積算値(平均を取るため)
            int   AccumSamples = 0;
        };
        static constexpr size_t kSectionCount = static_cast<size_t>(ePerfSection::Count);
        SectionState mSections[kSectionCount];

        // RegisterImgui()内、ComponentSystemManagerから集めたSystem別所要時間の一時バッファ。
        // 毎回clear()して再利用する(毎フレームのvector生成禁止のため)
        struct SystemTimingEntry
        {
            const char* PhaseName = nullptr;
            std::string SystemName;
            float       Ms = 0.0f;
        };
        std::vector<SystemTimingEntry> mSystemTimingEntries;

        // 素材別エフェクト負荷の表示用一時バッファ(ソートするためコピーが要る)。
        // mSystemTimingEntriesと同じくclear()して再利用する
        std::vector<graphics::EffekseerManager::EffectStatEntry> mEffectStatEntries;
    };
}
