#pragma once

#include<memory>
#include<string>

namespace data
{
    struct WaveData;
    template<typename T> class DataInspector;
}

namespace debug
{
    /// <summary>
    /// ウェーブサバイバルのコアループ用バランス調整データ(WaveData)のデバッグパネル。
    /// DataInspectorによるテーブル編集(CSV/DB・Id含む全フィールド)に加え、
    /// 編集内容を実行中のWaveComponentへ再適用する機能を持つ。
    /// IUserSystemは継承せず、生成時にImGuiManagerへ登録・破棄時に解除する。
    /// </summary>
    class WaveDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager 登録・解除に使うキー（シーンごとに一意にすること）</param>
        explicit WaveDebugPanel(std::string debugKey = "WaveDebug");
        ~WaveDebugPanel();

        WaveDebugPanel(const WaveDebugPanel&) = delete;
        WaveDebugPanel& operator=(const WaveDebugPanel&) = delete;

    private:
        void Draw();
        void ApplyToRunningWave();

        // 唯一のテーブルエディタ（Id含む全フィールド編集・CSV/DB操作）
        std::unique_ptr<data::DataInspector<data::WaveData>> mInspector;

        // ImGuiManager 登録・解除に使うキー
        std::string mDebugKey;
    };
}
