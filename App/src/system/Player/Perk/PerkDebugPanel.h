#pragma once

#include<memory>
#include<string>

namespace data
{
    struct PerkData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///パークのバランス調整データPerkDataのデバッグパネル。各パーク種別の抽選重みと選択可能回数をGUIで編集しCSV/DBへ保存できる
    ///</summary>
    class PerkDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー。シーンごとに一意にすること</param>
        explicit PerkDebugPanel(std::string debugKey = "PerkDebug");
        ~PerkDebugPanel();

        PerkDebugPanel(const PerkDebugPanel&) = delete;
        PerkDebugPanel& operator=(const PerkDebugPanel&) = delete;

    private:
        void Draw();

        ///<summary>
        ///テーブルエディタ、Id含む全フィールド編集とCSV/DB操作
        ///</summary>
        std::unique_ptr<data::DataInspector<data::PerkData>> mInspector;

        ///<summary>
        ///ImGuiManager登録・解除に使うキー
        ///</summary>
        std::string mDebugKey;
    };
}
