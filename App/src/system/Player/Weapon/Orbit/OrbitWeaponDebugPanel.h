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
    ///<summary>
    ///Orbit、SelfDefense型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class OrbitWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
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
