#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
    struct ENGINE_API PlayerMovementComponent
    {
        float MaxSpeed = 110.0f;     // 最大速度
        float Acceleration = 100.0f; // 加速度
        float Deceleration = 20.0f;  // 減速度

        DirectX::XMFLOAT3 MoveInput = { 0.f, 0.f, 0.f }; // 移動入力

        float CurrentSpeed = 0.0f; // 現在の速度

        bool UseAcceleration = false; // true:加減速を使う false:入力に応じて即座に速度が変わる
    };
}
