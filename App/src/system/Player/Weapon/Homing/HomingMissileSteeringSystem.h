#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    ///<summary>
    ///追尾弾の毎フレーム操舵を行うシステム。対象が無効になった場合はHomingSearchRadius内で最も近い敵を再捕捉する
    ///</summary>
    class HomingMissileSteeringSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///DirectionをtargetPos方向へTurnSpeed×deltaTimeの範囲内で回転させる、XZ平面のみ
        ///</summary>
        static void SteerTowards(
            DirectX::XMFLOAT3& direction,
            const DirectX::XMFLOAT3& fromPos,
            const DirectX::XMFLOAT3& targetPos,
            float turnSpeedDegrees,
            float deltaTime);
    };
}
