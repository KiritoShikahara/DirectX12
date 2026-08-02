#pragma once

#include<memory>
#include<string>

namespace data
{
    struct BoneSpearWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///BoneSpear型武器マスタデータのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class BoneSpearWeaponDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit BoneSpearWeaponDebugPanel(std::string debugKey = "BoneSpearWeaponDebug");
        ~BoneSpearWeaponDebugPanel();

        BoneSpearWeaponDebugPanel(const BoneSpearWeaponDebugPanel&) = delete;
        BoneSpearWeaponDebugPanel& operator=(const BoneSpearWeaponDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::BoneSpearWeaponData>> mInspector;
        std::string mDebugKey;
    };
}
