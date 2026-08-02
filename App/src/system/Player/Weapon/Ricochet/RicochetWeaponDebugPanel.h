#pragma once

#include<memory>
#include<string>

namespace data
{
    struct RicochetWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///Ricochet型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class RicochetWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit RicochetWeaponDebugPanel(std::string debugKey = "RicochetWeaponDebug");
        ~RicochetWeaponDebugPanel();

        RicochetWeaponDebugPanel(const RicochetWeaponDebugPanel&) = delete;
        RicochetWeaponDebugPanel& operator=(const RicochetWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::RicochetWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
