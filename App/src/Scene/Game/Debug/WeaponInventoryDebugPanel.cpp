#include "apppch.h"
#include "WeaponInventoryDebugPanel.h"

#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<Scene/Game/Factory/GameSceneFactory.h>
#include<Tag/EntityTag.h>

namespace
{
    struct KnownWeapon
    {
        const char* DisplayName;
        ecs::eWeaponType Type;
        int WeaponId;
        ecs::eWeaponControl Control;
    };

    // デバッグパネルが認識する武器一覧。新しい武器種別を追加したらここにも追記すること。
    constexpr KnownWeapon kKnownWeapons[] =
    {
        { "FireBolt (SingleShot)",  ecs::eWeaponType::SingleShot,  0, ecs::eWeaponControl::Manual },
        { "IceSpike (AreaAttack)",  ecs::eWeaponType::AreaAttack,  0, ecs::eWeaponControl::Manual },
        { "FrostOrb (SelfDefense)", ecs::eWeaponType::SelfDefense, 0, ecs::eWeaponControl::Auto },
        { "Nova",                   ecs::eWeaponType::Nova,        0, ecs::eWeaponControl::Auto },
        { "Homing Missile",         ecs::eWeaponType::Homing,      0, ecs::eWeaponControl::Auto },
        { "Chain Lightning",        ecs::eWeaponType::Chain,       0, ecs::eWeaponControl::Auto },
        { "Meteor",                 ecs::eWeaponType::Meteor,      0, ecs::eWeaponControl::Auto },
        { "Void Beam",              ecs::eWeaponType::VoidBeam,    0, ecs::eWeaponControl::Auto },
        { "Bone Spear",             ecs::eWeaponType::BoneSpear,   0, ecs::eWeaponControl::Auto },
        { "Cleave",                 ecs::eWeaponType::Cleave,      0, ecs::eWeaponControl::Auto },
        { "Flicker Strike",         ecs::eWeaponType::FlickerStrike, 0, ecs::eWeaponControl::Manual },
    };

    const char* WeaponTypeName(ecs::eWeaponType type)
    {
        switch (type)
        {
        case ecs::eWeaponType::SingleShot:  return "SingleShot";
        case ecs::eWeaponType::AreaAttack:  return "AreaAttack";
        case ecs::eWeaponType::SelfDefense: return "SelfDefense";
        case ecs::eWeaponType::Nova:        return "Nova";
        case ecs::eWeaponType::Homing:      return "Homing";
        case ecs::eWeaponType::Chain:       return "Chain";
        case ecs::eWeaponType::Meteor:      return "Meteor";
        case ecs::eWeaponType::VoidBeam:    return "VoidBeam";
        case ecs::eWeaponType::BoneSpear:   return "BoneSpear";
        case ecs::eWeaponType::Cleave:      return "Cleave";
        case ecs::eWeaponType::FlickerStrike: return "FlickerStrike";
        default:                            return "Unknown";
        }
    }
}

namespace debug
{
    WeaponInventoryDebugPanel::WeaponInventoryDebugPanel(std::string debugKey)
        : mDebugKey(std::move(debugKey))
    {
#ifdef _DEBUG
        sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
    }

    WeaponInventoryDebugPanel::~WeaponInventoryDebugPanel()
    {
#ifdef _DEBUG
        sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
    }

#ifdef _DEBUG
    void WeaponInventoryDebugPanel::Draw()
    {
        if (!ImGui::Begin("Weapon Inventory Debug"))
        {
            ImGui::End();
            return;
        }

        auto& registry = ecs::EntityManager::Get().GetRegistry();
        auto playerView = registry.view<ecs::PlayerTag, ecs::WeaponInventoryComponent>();
        if (playerView.begin() == playerView.end())
        {
            ImGui::TextDisabled("No player.");
            ImGui::End();
            return;
        }

        const entt::entity playerEntity = *playerView.begin();
        auto& inventory = registry.get<ecs::WeaponInventoryComponent>(playerEntity);

        ImGui::Text("Slots: %d / %d", inventory.Count(), inventory.MaxSlots);
        ImGui::Separator();

        ImGui::TextUnformatted("Add:");
        for (const auto& known : kKnownWeapons)
        {
            ImGui::PushID(known.DisplayName);
            ImGui::BeginDisabled(!inventory.HasFreeSlot());
            if (ImGui::Button(known.DisplayName))
            {
                ::ecs::GameSceneFactory::AddWeaponToPlayer(playerEntity, known.Type, known.WeaponId, known.Control);
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Owned (click to remove):");

        entt::entity removeTarget = entt::null;
        for (entt::entity weaponEntity : inventory.Weapons)
        {
            if (!registry.valid(weaponEntity)) continue;
            const auto* weapon = registry.try_get<ecs::WeaponComponent>(weaponEntity);
            if (weapon == nullptr) continue;

            ImGui::PushID(static_cast<int>(weaponEntity));
            if (ImGui::Button("Remove"))
            {
                removeTarget = weaponEntity;
            }
            ImGui::SameLine();
            ImGui::Text("%s (Lv%d, WeaponID=%d)", WeaponTypeName(weapon->Type), weapon->Level, weapon->WeaponID);
            ImGui::PopID();
        }

        // ループ中の削除はinventory.Weaponsを書き換えてしまいイテレータを不正化するため、
        // 走査完了後にまとめて行う
        if (removeTarget != entt::null)
        {
            ::ecs::GameSceneFactory::RemoveWeaponFromPlayer(playerEntity, removeTarget);
        }

        ImGui::End();
    }
#else
    void WeaponInventoryDebugPanel::Draw() {}
#endif
}
