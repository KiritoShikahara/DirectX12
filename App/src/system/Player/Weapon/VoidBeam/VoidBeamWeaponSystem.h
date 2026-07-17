#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct VoidBeamWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    /// <summary>
    /// VoidBeam型武器(WeaponComponent::Type == VoidBeam)の発動ロジック。
    /// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の最も近い敵の方向へ
    /// 直線状のビームを放つ完全自動の武器。プレイヤー位置からBeamLength・BeamWidthで
    /// 定義される直線範囲内にいる敵全員へ、跳躍(Chain Lightning)のような対象数制限・
    /// 減衰を挟まず同時にダメージを与える（貫通が本武器のコンセプト）。
    /// Chain Lightningと同様、移動する実体を持たないため命中は瞬時に解決される。
    /// </summary>
    class VoidBeamWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>directionへ向けてBeamLength・BeamWidthの直線範囲内にいる敵全員へダメージを与える</summary>
        static void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& direction,
            const data::VoidBeamWeaponData& masterData);
    };
}
