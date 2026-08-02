#pragma once

#include<memory>
#include<string>

namespace data
{
    struct ChainLightningWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///ChainLightning型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class ChainLightningWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit ChainLightningWeaponDebugPanel(std::string debugKey = "ChainLightningWeaponDebug");
        ~ChainLightningWeaponDebugPanel();

        ChainLightningWeaponDebugPanel(const ChainLightningWeaponDebugPanel&) = delete;
        ChainLightningWeaponDebugPanel& operator=(const ChainLightningWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::ChainLightningWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
