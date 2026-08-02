#pragma once

#include<memory>
#include<string>

namespace data
{
    struct BossData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///ボス敵のマスタデータBossDataのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class BossDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー。シーンごとに一意にすること</param>
        explicit BossDebugPanel(std::string debugKey = "BossDebug");
        ~BossDebugPanel();

        BossDebugPanel(const BossDebugPanel&) = delete;
        BossDebugPanel& operator=(const BossDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::BossData>> mInspector;
        std::string mDebugKey;
    };
}
