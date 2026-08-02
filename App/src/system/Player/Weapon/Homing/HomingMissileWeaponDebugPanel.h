#pragma once

#include<memory>
#include<string>

namespace data
{
    struct HomingMissileWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///Homing Missile型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class HomingMissileWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit HomingMissileWeaponDebugPanel(std::string debugKey = "HomingMissileWeaponDebug");
        ~HomingMissileWeaponDebugPanel();

        HomingMissileWeaponDebugPanel(const HomingMissileWeaponDebugPanel&) = delete;
        HomingMissileWeaponDebugPanel& operator=(const HomingMissileWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::HomingMissileWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
