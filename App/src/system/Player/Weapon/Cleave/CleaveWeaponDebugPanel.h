#pragma once

#include<memory>
#include<string>

namespace data
{
    struct CleaveWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///Cleave型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class CleaveWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit CleaveWeaponDebugPanel(std::string debugKey = "CleaveWeaponDebug");
        ~CleaveWeaponDebugPanel();

        CleaveWeaponDebugPanel(const CleaveWeaponDebugPanel&) = delete;
        CleaveWeaponDebugPanel& operator=(const CleaveWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::CleaveWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
