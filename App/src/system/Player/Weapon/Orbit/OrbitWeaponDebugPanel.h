#pragma once

#include<memory>
#include<string>

namespace data
{
    struct OrbitWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    /// <summary>
    /// Orbit(SelfDefense)型武器のマスタデータ(OrbitWeaponData)のデバッグパネル。
    /// 発動のたびにDATA_MGRから直接マスタデータを取得する設計のため、Apply操作は不要で
    /// DataInspectorによるテーブル編集(CSV/DB・Id含む全フィールド)をそのまま提供すれば良い
    /// (SingleShotWeaponDebugPanelと同じ方針)。
    /// IUserSystemは継承せず、生成時にImGuiManagerへ登録・破棄時に解除する。
    /// </summary>
    class OrbitWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager 登録・解除に使うキー（シーンごとに一意にすること）</param>
        explicit OrbitWeaponDebugPanel(std::string debugKey = "OrbitWeaponDebug");
        ~OrbitWeaponDebugPanel();

        OrbitWeaponDebugPanel(const OrbitWeaponDebugPanel&) = delete;
        OrbitWeaponDebugPanel& operator=(const OrbitWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::OrbitWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
