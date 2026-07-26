#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct BoneSpearWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    /// <summary>
    /// BoneSpear型武器の発射ロジック。完全自動で、FireInterval秒ごとにSearchRadius内の
    /// 最寄りの敵へ貫通弾(PierceCount設定済み)を1体撃つ。以降の移動・命中判定は
    /// Projectile系の共通パイプラインに任せる。
    /// </summary>
    class BoneSpearWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>最も近い敵の方向へ ProjectileComponent エンティティを1体生成する</summary>
        static void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const DirectX::XMFLOAT3& direction,
            const data::BoneSpearWeaponData& masterData);
    };
}
