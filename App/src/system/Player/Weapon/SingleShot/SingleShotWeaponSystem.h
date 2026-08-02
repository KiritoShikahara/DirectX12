#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct SingleShotWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    ///<summary>
    ///SingleShot型武器の発射ロジック。クールダウンが明けるたびに所有者の狙い方向へProjectileComponentを生成する
    ///</summary>
    class SingleShotWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///狙い方向へProjectileComponentエンティティを1体生成する、shotCountが複数なら扇状に広げる
        ///</summary>
        static void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const data::SingleShotWeaponData& masterData,
            int shotIndex = 0,
            int shotCount = 1);
    };
}
