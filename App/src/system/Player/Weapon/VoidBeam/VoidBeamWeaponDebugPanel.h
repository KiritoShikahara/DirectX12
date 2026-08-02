#pragma once

#include<memory>
#include<string>

namespace data
{
    struct VoidBeamWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///VoidBeam型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class VoidBeamWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit VoidBeamWeaponDebugPanel(std::string debugKey = "VoidBeamWeaponDebug");
        ~VoidBeamWeaponDebugPanel();

        VoidBeamWeaponDebugPanel(const VoidBeamWeaponDebugPanel&) = delete;
        VoidBeamWeaponDebugPanel& operator=(const VoidBeamWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::VoidBeamWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
