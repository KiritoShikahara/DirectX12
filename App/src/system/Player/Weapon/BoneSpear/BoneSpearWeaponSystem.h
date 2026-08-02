#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct BoneSpearWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    ///<summary>
    ///BoneSpear型武器の発射ロジック。完全自動でSearchRadius内の最寄りの敵へ貫通弾を1体撃つ
    ///</summary>
    class BoneSpearWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///最も近い敵の方向へProjectileComponentエンティティを1体生成する
        ///</summary>
        static void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const DirectX::XMFLOAT3& direction,
            const data::BoneSpearWeaponData& masterData);
    };
}
