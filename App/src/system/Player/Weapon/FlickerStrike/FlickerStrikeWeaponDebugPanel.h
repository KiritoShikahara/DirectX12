#pragma once

#include<memory>
#include<string>

namespace data
{
    struct FlickerStrikeWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///FlickerStrike型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class FlickerStrikeWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit FlickerStrikeWeaponDebugPanel(std::string debugKey = "FlickerStrikeWeaponDebug");
        ~FlickerStrikeWeaponDebugPanel();

        FlickerStrikeWeaponDebugPanel(const FlickerStrikeWeaponDebugPanel&) = delete;
        FlickerStrikeWeaponDebugPanel& operator=(const FlickerStrikeWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::FlickerStrikeWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
