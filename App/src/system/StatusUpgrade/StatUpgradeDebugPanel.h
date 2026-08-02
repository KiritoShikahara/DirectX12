#pragma once

#include<memory>
#include<string>

namespace data
{
    struct StatUpgradeData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///ステータス強化のマスタデータStatUpgradeDataのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class StatUpgradeDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー。シーンごとに一意にすること</param>
        explicit StatUpgradeDebugPanel(std::string debugKey = "StatUpgradeDebug");
        ~StatUpgradeDebugPanel();

        StatUpgradeDebugPanel(const StatUpgradeDebugPanel&) = delete;
        StatUpgradeDebugPanel& operator=(const StatUpgradeDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::StatUpgradeData>> mInspector;
        std::string mDebugKey;
    };
}
