#pragma once

#include<entt/entt.hpp>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct CleaveWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    /// <summary>
    /// Cleave型武器(WeaponComponent::Type == Cleave)の発動ロジック。
    /// 発動トリガーが無く、FireInterval秒ごとに所有者の狙い方向
    /// (PlayerAimComponent::Direction)を中心とした扇状範囲(Radius系×ConeAngleDegrees)
    /// 内にいる敵全員へ、即座に近接ダメージとノックバックを与える完全自動の武器。
    /// ノックバックはEnemyKnockbackComponentを付与することで実現し、
    /// 実際の速度適用・持続時間管理はEnemyKnockbackSystemが担当する。
    /// </summary>
    class CleaveWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>発動: 狙い方向の扇状範囲内にいる敵全員へダメージ・ノックバックを与える</summary>
        void Swing(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const data::CleaveWeaponData& masterData);

        // Swing()のOverlapSphere結果の一時バッファ。毎回clear()して再利用する
        // (毎フレーム相当のvector生成禁止のため。攻撃回数パークで1フレーム内に複数回呼ばれうる)
        std::vector<entt::entity> mOverlapped;
    };
}
