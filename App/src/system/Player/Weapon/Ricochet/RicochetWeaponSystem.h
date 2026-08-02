#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct WeaponComponent; }
namespace data { struct RicochetWeaponData; }

namespace ecs
{
    ///<summary>
    ///Ricochet型武器を処理するシステム。SearchRadius内に敵がいる場合のみ球体の弾を発射する完全自動の武器。増殖処理はProjectileCollisionSystemが担当する
    ///</summary>
    class RicochetWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///狙い方向へ球体の弾を1体生成する、shotCountが複数なら扇状に広げる
        ///</summary>
        static void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            entt::entity target,
            const data::RicochetWeaponData& masterData,
            int shotIndex = 0,
            int shotCount = 1);
    };
}
