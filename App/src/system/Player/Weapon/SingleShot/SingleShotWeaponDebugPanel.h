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
    ///<summary>
    ///SingleShot型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class SingleShotWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
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
