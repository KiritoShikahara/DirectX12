#include "apppch.h"
#include "WeaponMasterDataDebugPanel.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include

#include<Data/Weapon/SingleShotWeaponData.h>
#include<Data/Weapon/AreaAttackWeaponData.h>
#include<Data/Weapon/BoneSpearWeaponData.h>
#include<Data/Weapon/ChainLightningWeaponData.h>
#include<Data/Weapon/CleaveWeaponData.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>
#include<Data/Weapon/HomingMissileWeaponData.h>
#include<Data/Weapon/MeteorWeaponData.h>
#include<Data/Weapon/NovaWeaponData.h>
#include<Data/Weapon/OrbitWeaponData.h>
#include<Data/Weapon/RicochetWeaponData.h>
#include<Data/Weapon/VoidBeamWeaponData.h>

namespace debug
{
    WeaponMasterDataDebugPanel::WeaponMasterDataDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#if DEV_TOOL_ENABLED
        auto& reg = data::DataRegistry::Get();
        mSingleShotInspector = std::make_unique<data::DataInspector<data::SingleShotWeaponData>>(reg.GetManager<data::SingleShotWeaponData>());
        mAreaAttackInspector = std::make_unique<data::DataInspector<data::AreaAttackWeaponData>>(reg.GetManager<data::AreaAttackWeaponData>());
        mBoneSpearInspector = std::make_unique<data::DataInspector<data::BoneSpearWeaponData>>(reg.GetManager<data::BoneSpearWeaponData>());
        mChainLightningInspector = std::make_unique<data::DataInspector<data::ChainLightningWeaponData>>(reg.GetManager<data::ChainLightningWeaponData>());
        mCleaveInspector = std::make_unique<data::DataInspector<data::CleaveWeaponData>>(reg.GetManager<data::CleaveWeaponData>());
        mFlickerStrikeInspector = std::make_unique<data::DataInspector<data::FlickerStrikeWeaponData>>(reg.GetManager<data::FlickerStrikeWeaponData>());
        mHomingMissileInspector = std::make_unique<data::DataInspector<data::HomingMissileWeaponData>>(reg.GetManager<data::HomingMissileWeaponData>());
        mMeteorInspector = std::make_unique<data::DataInspector<data::MeteorWeaponData>>(reg.GetManager<data::MeteorWeaponData>());
        mNovaInspector = std::make_unique<data::DataInspector<data::NovaWeaponData>>(reg.GetManager<data::NovaWeaponData>());
        mOrbitInspector = std::make_unique<data::DataInspector<data::OrbitWeaponData>>(reg.GetManager<data::OrbitWeaponData>());
        mRicochetInspector = std::make_unique<data::DataInspector<data::RicochetWeaponData>>(reg.GetManager<data::RicochetWeaponData>());
        mVoidBeamInspector = std::make_unique<data::DataInspector<data::VoidBeamWeaponData>>(reg.GetManager<data::VoidBeamWeaponData>());

        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    WeaponMasterDataDebugPanel::~WeaponMasterDataDebugPanel()
    {
#if DEV_TOOL_ENABLED
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#if DEV_TOOL_ENABLED
    void WeaponMasterDataDebugPanel::Draw()
    {
        // 各セクションはCollapsingHeaderで折りたたみ/展開できる。デフォルトは全て閉じておき、
        // 必要な武器種別だけ開いてもらう(12種すべて展開すると結局長大になるため)。
        ImGui::SetNextWindowSize(ImVec2(720.f, 640.f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Weapon Master Data"))
        {
            ImGui::End();
            return;
        }

        auto section = [](const char* label, auto& inspector)
            {
                if (ImGui::CollapsingHeader(label))
                {
                    ImGui::PushID(label);
                    ImGui::Indent();
                    inspector->DrawContent();
                    ImGui::Unindent();
                    ImGui::PopID();
                }
            };

        section("SingleShot", mSingleShotInspector);
        section("AreaAttack", mAreaAttackInspector);
        section("BoneSpear", mBoneSpearInspector);
        section("ChainLightning", mChainLightningInspector);
        section("Cleave", mCleaveInspector);
        section("FlickerStrike", mFlickerStrikeInspector);
        section("HomingMissile", mHomingMissileInspector);
        section("Meteor", mMeteorInspector);
        section("Nova", mNovaInspector);
        section("Orbit", mOrbitInspector);
        section("Ricochet", mRicochetInspector);
        section("VoidBeam", mVoidBeamInspector);

        ImGui::End();
    }
#else
    void WeaponMasterDataDebugPanel::Draw() {}
#endif
}
