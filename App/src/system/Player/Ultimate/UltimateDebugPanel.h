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
    ///<summary>
    ///必殺技マスタデータUltimateDataのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class UltimateDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー。シーンごとに一意にすること</param>
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
