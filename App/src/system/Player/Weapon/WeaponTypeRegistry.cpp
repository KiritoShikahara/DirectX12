#include "apppch.h"
#include "WeaponTypeRegistry.h"

#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/SingleShot/SingleShotWeaponRuntimeComponent.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Orbit/OrbitWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Nova/NovaWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Homing/HomingMissileRuntimeComponent.h>
#include<system/Player/Weapon/ChainLightning/ChainLightningRuntimeComponent.h>
#include<system/Player/Weapon/Meteor/MeteorWeaponRuntimeComponent.h>
#include<system/Player/Weapon/VoidBeam/VoidBeamRuntimeComponent.h>
#include<system/Player/Weapon/BoneSpear/BoneSpearRuntimeComponent.h>
#include<system/Player/Weapon/Cleave/CleaveRuntimeComponent.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeRuntimeComponent.h>
#include<system/Player/Weapon/Ricochet/RicochetRuntimeComponent.h>

#include<functional>
#include<unordered_map>

namespace
{
    using AddRuntimeComponentFn = std::function<void(entt::registry&, entt::entity)>;

    const std::unordered_map<ecs::eWeaponType, AddRuntimeComponentFn>& GetRegistry()
    {
        // 新しい武器種別を追加する場合はここへ1行追記する
        static const std::unordered_map<ecs::eWeaponType, AddRuntimeComponentFn> table =
        {
            { ecs::eWeaponType::SingleShot,    [](entt::registry& r, entt::entity e) { r.emplace<ecs::SingleShotWeaponRuntimeComponent>(e); } },
            { ecs::eWeaponType::AreaAttack,    [](entt::registry& r, entt::entity e) { r.emplace<ecs::AreaAttackWeaponRuntimeComponent>(e); } },
            { ecs::eWeaponType::SelfDefense,   [](entt::registry& r, entt::entity e) { r.emplace<ecs::OrbitWeaponRuntimeComponent>(e); } },
            { ecs::eWeaponType::Nova,          [](entt::registry& r, entt::entity e) { r.emplace<ecs::NovaWeaponRuntimeComponent>(e); } },
            { ecs::eWeaponType::Homing,        [](entt::registry& r, entt::entity e) { r.emplace<ecs::HomingMissileRuntimeComponent>(e); } },
            { ecs::eWeaponType::Chain,         [](entt::registry& r, entt::entity e) { r.emplace<ecs::ChainLightningRuntimeComponent>(e); } },
            { ecs::eWeaponType::Meteor,        [](entt::registry& r, entt::entity e) { r.emplace<ecs::MeteorWeaponRuntimeComponent>(e); } },
            { ecs::eWeaponType::VoidBeam,      [](entt::registry& r, entt::entity e) { r.emplace<ecs::VoidBeamRuntimeComponent>(e); } },
            { ecs::eWeaponType::BoneSpear,     [](entt::registry& r, entt::entity e) { r.emplace<ecs::BoneSpearRuntimeComponent>(e); } },
            { ecs::eWeaponType::Cleave,        [](entt::registry& r, entt::entity e) { r.emplace<ecs::CleaveRuntimeComponent>(e); } },
            { ecs::eWeaponType::FlickerStrike, [](entt::registry& r, entt::entity e) { r.emplace<ecs::FlickerStrikeRuntimeComponent>(e); } },
            { ecs::eWeaponType::Ricochet,      [](entt::registry& r, entt::entity e) { r.emplace<ecs::RicochetRuntimeComponent>(e); } },
        };
        return table;
    }
}

namespace ecs::weaponutil
{
    void AddWeaponRuntimeComponent(entt::registry& registry, eWeaponType type, entt::entity weaponEntity)
    {
        const auto& table = GetRegistry();
        const auto it = table.find(type);
        if (it != table.end())
        {
            it->second(registry, weaponEntity);
        }
    }
}
