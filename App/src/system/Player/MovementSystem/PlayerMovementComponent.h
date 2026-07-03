#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
    struct ENGINE_API PlayerMovementComponent
    {
        float MaxSpeed = 5.0f; // 最大速度
        float Acceleration = 100.0f; // 加速度
        float Deceleration = 20.0f; // 減衰量

        DirectX::XMFLOAT3 MoveInput = { 0.f, 0.f, 0.f }; // 移動量

        float CurrentSpeed = 0.0f; // 今の速度

        bool UseAcceleration = false; // 車などで加速度を使用するかどうか true:使用する
    };
}