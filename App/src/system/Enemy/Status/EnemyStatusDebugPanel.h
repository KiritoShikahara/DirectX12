#pragma once

#include<memory>
#include<string>

namespace data
{
    struct EnemyData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///敵マスタデータEnemyDataのデバッグパネル。既存のDataInspectorによるテーブル編集に加え、編集内容を生存中の敵へ再適用する機能を持つ
    ///</summary>
    class EnemyStatusDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー。シーンごとに一意にすること</param>
        explicit EnemyStatusDebugPanel(std::string debugKey = "EnemyStatusDebug");
        ~EnemyStatusDebugPanel();

        EnemyStatusDebugPanel(const EnemyStatusDebugPanel&) = delete;
        EnemyStatusDebugPanel& operator=(const EnemyStatusDebugPanel&) = delete;

    private:
        void Draw();
        void ApplyAllToEnemies();
        void ApplyRowToEnemies(const data::EnemyData& row);

        ///<summary>
        ///既存のテーブルエディタ、Id含む全フィールド編集とCSV/DB操作を内包
        ///</summary>
        std::unique_ptr<data::DataInspector<data::EnemyData>> mInspector;

        ///<summary>
        ///ImGuiManager登録・解除に使うキー
        ///</summary>
        std::string mDebugKey;
    };
}
