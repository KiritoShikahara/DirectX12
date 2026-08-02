#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
    ///<summary>
    ///InGame中、敵とプレイヤーの接触でプレイヤーへダメージを与えるシステム。同一敵とのクールタイムで多段・連続ヒットを防ぐ
    ///</summary>
    class PlayerContactDamageSystem : public IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        static constexpr float kDefenseHalfPoint = 50.0f;
    };
}
