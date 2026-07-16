#pragma once

#include<memory>
#include<string>

namespace data
{
    struct UltimateData;
    template<typename T> class DataInspector;
}

namespace debug
{
    /// <summary>
    /// 必殺技(Ultimate)マスタデータ(UltimateData)のデバッグパネル。
    /// PlayerUltimateSystemは発動のたびにDATA_MGRから直接マスタデータを取得する設計のため
    /// (EnemyStatusDebugPanelと異なりランタイムへのキャッシュが無い)、Apply操作は不要で
    /// DataInspectorによるテーブル編集(CSV/DB・Id含む全フィールド)をそのまま提供すれば良い。
    /// これにより上昇する高さ(RiseHeight)等をGUIから即座に調整・確認できる。
    /// IUserSystemは継承せず、生成時にImGuiManagerへ登録・破棄時に解除する。
    /// </summary>
    class UltimateDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager 登録・解除に使うキー（シーンごとに一意にすること）</param>
        explicit UltimateDebugPanel(std::string debugKey = "UltimateDebug");
        ~UltimateDebugPanel();

        UltimateDebugPanel(const UltimateDebugPanel&) = delete;
        UltimateDebugPanel& operator=(const UltimateDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::UltimateData>> mInspector;
        std::string mDebugKey;
    };
}
