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
    ///<summary>
    ///ウェーブサバイバルのコアループ用バランス調整データWaveDataのデバッグパネル。DataInspectorでのテーブル編集に加え編集内容を実行中のWaveComponentへ再適用する機能を持つ
    ///</summary>
    class WaveDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー。シーンごとに一意にすること</param>
        explicit WaveDebugPanel(std::string debugKey = "WaveDebug");
        ~WaveDebugPanel();

        WaveDebugPanel(const WaveDebugPanel&) = delete;
        WaveDebugPanel& operator=(const WaveDebugPanel&) = delete;

    private:
        void Draw();
        void ApplyToRunningWave();

        ///<summary>
        ///唯一のテーブルエディタ、Id含む全フィールド編集とCSV/DB操作
        ///</summary>
        std::unique_ptr<data::DataInspector<data::WaveData>> mInspector;

        ///<summary>
        ///ImGuiManager登録・解除に使うキー
        ///</summary>
        std::string mDebugKey;
    };
}
