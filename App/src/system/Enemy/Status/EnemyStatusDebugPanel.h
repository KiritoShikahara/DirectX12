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
    /// <summary>
    /// 敵マスタ(EnemyData)のデバッグパネル。
    /// 既存 DataInspector によるテーブル編集(CSV/DB・Id含む全フィールド)に加え、
    /// 編集内容を生存中の敵(EnemyStatusComponent)へ再適用する機能を持つ。
    /// IUserSystem は継承せず、生成時に ImGuiManager へ登録・破棄時に解除する。
    /// </summary>
    class EnemyStatusDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager 登録・解除に使うキー（シーンごとに一意にすること）</param>
        explicit EnemyStatusDebugPanel(std::string debugKey = "EnemyStatusDebug");
        ~EnemyStatusDebugPanel();

        EnemyStatusDebugPanel(const EnemyStatusDebugPanel&) = delete;
        EnemyStatusDebugPanel& operator=(const EnemyStatusDebugPanel&) = delete;

    private:
        void Draw();
        void ApplyAllToEnemies();
        void ApplyRowToEnemies(const data::EnemyData& row);

        // 既存のテーブルエディタ（Id含む全フィールド編集・CSV/DB操作を内包）
        std::unique_ptr<data::DataInspector<data::EnemyData>> mInspector;

        // ImGuiManager 登録・解除に使うキー
        std::string mDebugKey;
    };
}