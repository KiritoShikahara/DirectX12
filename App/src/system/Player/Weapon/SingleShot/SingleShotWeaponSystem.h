#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct SingleShotWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    /// <summary>
    /// SingleShot型武器(WeaponComponent::Type == SingleShot)の発射ロジック。
    /// Auto制御の武器はクールダウンが明けるたびに、所有者(Owner)の
    /// PlayerAimComponent::Direction へ向けて ProjectileComponent を生成する。
    /// ダメージ・爆発半径は SingleShotWeaponData(マスタ) と WeaponComponent::Level から算出する。
    /// </summary>
    class SingleShotWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>狙い方向へ ProjectileComponent エンティティを1体生成する</summary>
        static void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const data::SingleShotWeaponData& masterData);
    };
}
