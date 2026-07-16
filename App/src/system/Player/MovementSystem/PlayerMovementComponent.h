#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
    struct ENGINE_API PlayerMovementComponent
    {
        float MaxSpeed = 75.0f; // �ő呬�x
        float Acceleration = 100.0f; // �����x
        float Deceleration = 20.0f; // ������

        DirectX::XMFLOAT3 MoveInput = { 0.f, 0.f, 0.f }; // �ړ���

        float CurrentSpeed = 0.0f; // ���̑��x

        bool UseAcceleration = false; // �ԂȂǂŉ����x���g�p���邩�ǂ��� true:�g�p����
    };
}