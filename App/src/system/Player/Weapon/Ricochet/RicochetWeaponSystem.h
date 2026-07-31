#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct WeaponComponent; }
namespace data { struct RicochetWeaponData; }

namespace ecs
{
    /// <summary>
    /// Ricochet型武器を処理するシステム。狙い不要の完全自動発動で、
    /// SearchRadius内に敵がいる場合のみFireInterval間隔で球体の弾を発射する
    /// （対象が見つからない間はクールダウンを消費せず待機する）。
    /// 発射した弾が敵に命中した際の増殖(Generation/SplitCount)はProjectileCollisionSystemが
    /// 汎用Projectileパイプラインの一部として担当する。本Systemは初弾の発射のみ行う。
    /// </summary>
    class RicochetWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>狙い方向(shotCount>1の場合は扇状に広げたshotIndex番目の方向)へ
        /// 球体の弾(ProjectileComponent、Generation=0)を1体生成する</summary>
        static void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            entt::entity target,
            const data::RicochetWeaponData& masterData,
            int shotIndex = 0,
            int shotCount = 1);
    };
}
