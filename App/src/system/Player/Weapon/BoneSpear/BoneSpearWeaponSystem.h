#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct BoneSpearWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    /// <summary>
    /// BoneSpear型武器(WeaponComponent::Type == BoneSpear)の発射ロジック。
    /// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の最も近い敵へ向けて
    /// ProjectileComponent(IsHoming=false、PierceCount設定済み)を1体発射する
    /// 完全自動の武器。発射後は誘導せず直進し、命中判定・貫通処理は既存のProjectile
    /// 汎用パイプライン(ProjectileMovementSystem/ProjectileCollisionSystem)をそのまま再利用する。
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
