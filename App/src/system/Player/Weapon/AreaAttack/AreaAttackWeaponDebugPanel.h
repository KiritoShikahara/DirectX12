#pragma once

#include<memory>
#include<string>

namespace data
{
    struct AreaAttackWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///AreaAttack型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class AreaAttackWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit AreaAttackWeaponDebugPanel(std::string debugKey = "AreaAttackWeaponDebug");
        ~AreaAttackWeaponDebugPanel();

        AreaAttackWeaponDebugPanel(const AreaAttackWeaponDebugPanel&) = delete;
        AreaAttackWeaponDebugPanel& operator=(const AreaAttackWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::AreaAttackWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
