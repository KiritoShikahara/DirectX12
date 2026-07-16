#pragma once

#include<memory>
#include<string>

namespace data
{
    struct SingleShotWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    /// <summary>
    /// SingleShot型武器マスタデータ(SingleShotWeaponData)のデバッグパネル。
    /// SingleShotWeaponSystemは発射のたびにDATA_MGRから直接マスタデータを取得する設計のため
    /// (EnemyStatusDebugPanelと異なりランタイムへのキャッシュが無い)、Apply操作は不要で
    /// DataInspectorによるテーブル編集(CSV/DB・Id含む全フィールド)をそのまま提供すれば良い。
    /// これにより、新しい武器行をCSVへ追加してもRelease版のDB(db.db)へ同期し忘れる問題を
    /// GUI上のLoad/Save操作で防げる。
    /// IUserSystemは継承せず、生成時にImGuiManagerへ登録・破棄時に解除する。
    /// </summary>
    class SingleShotWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager 登録・解除に使うキー（シーンごとに一意にすること）</param>
        explicit SingleShotWeaponDebugPanel(std::string debugKey = "SingleShotWeaponDebug");
        ~SingleShotWeaponDebugPanel();

        SingleShotWeaponDebugPanel(const SingleShotWeaponDebugPanel&) = delete;
        SingleShotWeaponDebugPanel& operator=(const SingleShotWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::SingleShotWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
