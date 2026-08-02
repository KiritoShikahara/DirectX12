#pragma once

#include<memory>
#include<string>

namespace data
{
    struct MeteorWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///Meteor型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class MeteorWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit MeteorWeaponDebugPanel(std::string debugKey = "MeteorWeaponDebug");
        ~MeteorWeaponDebugPanel();

        MeteorWeaponDebugPanel(const MeteorWeaponDebugPanel&) = delete;
        MeteorWeaponDebugPanel& operator=(const MeteorWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::MeteorWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
