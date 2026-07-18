#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    /// <summary>
    /// 追尾弾(ProjectileComponent::IsHoming == true)の毎フレーム操舵を行うシステム。
    /// 対象が無効になった場合はHomingSearchRadius内で最も近い敵を再捕捉する。
    /// ProjectileMovementSystem(Direction基準で速度を反映する)より前に実行すること。
    /// 通常の弾(IsHoming == false)には一切干渉しない。
    /// </summary>
    class HomingMissileSteeringSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>DirectionをtargetPos方向へTurnSpeed*deltaTimeの範囲内で回転させる（XZ平面のみ、Yは常に0）</summary>
        static void SteerTowards(
            DirectX::XMFLOAT3& direction,
            const DirectX::XMFLOAT3& fromPos,
            const DirectX::XMFLOAT3& targetPos,
            float turnSpeedDegrees,
            float deltaTime);
    };
}
