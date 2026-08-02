#pragma once

#include<memory>
#include<string>

namespace data
{
    struct NovaWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///Nova型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class NovaWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit NovaWeaponDebugPanel(std::string debugKey = "NovaWeaponDebug");
        ~NovaWeaponDebugPanel();

        NovaWeaponDebugPanel(const NovaWeaponDebugPanel&) = delete;
        NovaWeaponDebugPanel& operator=(const NovaWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::NovaWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
