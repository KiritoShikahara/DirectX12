#pragma once

#include<memory>
#include<string>

namespace data
{
    struct SingleShotWeaponData;
    struct AreaAttackWeaponData;
    struct BoneSpearWeaponData;
    struct ChainLightningWeaponData;
    struct CleaveWeaponData;
    struct FlickerStrikeWeaponData;
    struct HomingMissileWeaponData;
    struct MeteorWeaponData;
    struct NovaWeaponData;
    struct OrbitWeaponData;
    struct RicochetWeaponData;
    struct VoidBeamWeaponData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///全武器種別のマスタデータ編集パネルを、個別ウィンドウ乱立を避けて1つのウィンドウへ統合したもの
    ///</summary>
    class WeaponMasterDataDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー、シーンごとに一意にすること</param>
        explicit WeaponMasterDataDebugPanel(std::string debugKey = "WeaponMasterDataDebug");
        ~WeaponMasterDataDebugPanel();

        WeaponMasterDataDebugPanel(const WeaponMasterDataDebugPanel&) = delete;
        WeaponMasterDataDebugPanel& operator=(const WeaponMasterDataDebugPanel&) = delete;

    private:
        void Draw();

        std::string mDebugKey;

        std::unique_ptr<data::DataInspector<data::SingleShotWeaponData>> mSingleShotInspector;
        std::unique_ptr<data::DataInspector<data::AreaAttackWeaponData>> mAreaAttackInspector;
        std::unique_ptr<data::DataInspector<data::BoneSpearWeaponData>> mBoneSpearInspector;
        std::unique_ptr<data::DataInspector<data::ChainLightningWeaponData>> mChainLightningInspector;
        std::unique_ptr<data::DataInspector<data::CleaveWeaponData>> mCleaveInspector;
        std::unique_ptr<data::DataInspector<data::FlickerStrikeWeaponData>> mFlickerStrikeInspector;
        std::unique_ptr<data::DataInspector<data::HomingMissileWeaponData>> mHomingMissileInspector;
        std::unique_ptr<data::DataInspector<data::MeteorWeaponData>> mMeteorInspector;
        std::unique_ptr<data::DataInspector<data::NovaWeaponData>> mNovaInspector;
        std::unique_ptr<data::DataInspector<data::OrbitWeaponData>> mOrbitInspector;
        std::unique_ptr<data::DataInspector<data::RicochetWeaponData>> mRicochetInspector;
        std::unique_ptr<data::DataInspector<data::VoidBeamWeaponData>> mVoidBeamInspector;
    };
}
