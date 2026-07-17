#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct WeaponComponent; }
namespace data { struct HomingMissileWeaponData; }

namespace ecs
{
    /// <summary>
    /// Homing Missile型武器を処理するシステム。狙い不要の完全自動発動で、
    /// SearchRadius内に敵がいる場合のみFireInterval間隔で追尾弾を発射する
    /// （対象が見つからない間はクールダウンを消費せず待機する）。
    /// 発射する弾自体の移動・命中判定は既存のProjectile汎用パイプラインを再利用し、
    /// 追尾操舵はHomingMissileSteeringSystemが別途担当する。
    /// </summary>
    class HomingMissileWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>狙い方向(shotCount>1の場合は扇状に広げたshotIndex番目の方向)へ
        /// 追尾弾(ProjectileComponent、IsHoming=true)を1体生成する</summary>
        static void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            entt::entity target,
            const data::HomingMissileWeaponData& masterData,
            int shotIndex = 0,
            int shotCount = 1);
    };
}
