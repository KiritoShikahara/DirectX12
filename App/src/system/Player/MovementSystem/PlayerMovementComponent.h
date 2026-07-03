#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
    struct ENGINE_API PlayerMovementComponent
    {
        float MaxSpeed = 5.0f; // Å‘å‘¬“x
        float Acceleration = 30.0f; // ‰Á‘¬“x
        float Deceleration = 20.0f; // Œ¸Š—Ê

        DirectX::XMFLOAT3 MoveInput = { 0.f, 0.f, 0.f }; // ˆÚ“®—Ê

        float CurrentSpeed = 0.0f; // ¡‚Ì‘¬“x
    };
}