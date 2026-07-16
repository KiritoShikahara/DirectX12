#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct WeaponComponent; }
namespace data { struct NovaWeaponData; }

namespace ecs
{
    /// <summary>
    /// Nova型武器（プレイヤー自身を中心とした周期的な範囲ダメージ）を処理するシステム。
    /// 狙い・移動を必要としない完全自動の持続武器。InGame状態のときのみ動作する。
    /// </summary>
    class NovaWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>発動: 所有者中心に球形ダメージを与え、ワンショットエフェクトを再生する</summary>
        static void Pulse(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const data::NovaWeaponData& masterData);
    };
}
