#pragma once

#include<Data/Storage/Registry/DataRegistry.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<entt/entt.hpp>

namespace ecs
{
    ///<summary>
    ///プレイヤーが所持するパワーチャージ数。敵撃破ごとに加算されFlicker Strike等のチャージ消費スキルで全消費される。上限はFlickerStrikeWeaponDataで管理し、時間経過による自然減衰は行わない
    ///</summary>
    struct PlayerPowerChargeComponent
    {
        int Count = 0;
    };

    ///<summary>
    ///現在のパワーチャージ上限を求める。FlickerStrikeWeaponData::MaxCharge+MaxChargePerLevel×プレイヤーレベル-1
    ///</summary>
    inline int ComputeMaxPowerCharge(entt::registry& registry, entt::entity playerEntity)
    {
        const auto* masterData = DATA_MGR(data::FlickerStrikeWeaponData).GetById(data::kFlickerStrikeGlobalConfigId);
        if (masterData == nullptr) return 0;

        int level = 1;
        if (const auto* playerLevel = registry.try_get<PlayerLevelComponent>(playerEntity))
        {
            level = playerLevel->Level;
        }

        return masterData->MaxCharge + masterData->MaxChargePerLevel * (level - 1);
    }
}
