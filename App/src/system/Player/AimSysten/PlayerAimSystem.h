#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace sys
{
    ///<summary>
    ///攻撃方向を更新する。パッドは右スティック、マウスはカーソル方向を採用し、方向が定まらないフレームは前回値を維持する
    ///</summary>
    class PlayerAimSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>右スティックの傾きから攻撃方向を求める</summary>
        /// <returns>true:方向が確定した false:デッドゾーン内</returns>
        static bool TryGetPadAimDirection(DirectX::XMFLOAT3& outDirection);

        /// <summary>マウスカーソルの位置から攻撃方向を求める</summary>
        /// <returns>true:方向が確定した false:カーソルがプレイヤーとほぼ同位置</returns>
        static bool TryGetMouseAimDirection(
            entt::registry& registry,
            const DirectX::XMFLOAT3& playerPosition,
            DirectX::XMFLOAT3& outDirection);

        // これ未満の長さの二乗は無効な方向とみなす
        static constexpr float kMinDirectionLengthSq = 0.0001f;
    };
}