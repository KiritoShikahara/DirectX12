#pragma once

#include<Data/Storage/Registry/DataRegistry.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<entt/entt.hpp>

namespace ecs
{
    /// <summary>
    /// プレイヤーが所持するパワーチャージ数。敵撃破ごとに加算され(EnemyDeathSystem::
    /// AwardPowerCharge参照)、Flicker Strike等のチャージ消費スキルで全消費される。
    /// 上限(MaxCharge)はFlickerStrikeWeaponDataで管理する(現状の唯一の消費先のため。
    /// 将来チャージを使う武器が増えた場合は共有のコンフィグへ切り出す)。
    /// 時間経過による自然減衰は行わない(貯めたら消費するまで維持する)。
    /// </summary>
    struct PlayerPowerChargeComponent
    {
        int Count = 0;
    };

    /// <summary>
    /// 現在のパワーチャージ上限を求める：FlickerStrikeWeaponData::MaxCharge(Lv1時点の初期値)+
    /// MaxChargePerLevel×(プレイヤーレベル-1)。上限を参照する箇所(EnemyDeathSystem::
    /// AwardPowerCharge、GameStatusDebugPanel等)は重複計算を避けるためここを共通で使う。
    /// </summary>
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
