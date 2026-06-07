#pragma once
#include <DirectXMath.h>
#include <cstdint>

namespace ecs
{
    enum class eLightType : uint32_t
    {
        Directional = 0,
        Point = 1,
        Spot = 2,
    };

    struct DirectionalLightComponent
    {
        DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
        DirectX::XMFLOAT3 Color = { 1.0f,  1.0f, 1.0f };
        float             Intensity = 1.0f;
        bool              IsActive = true;
    };

    struct PointLightComponent
    {
        DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };
        float             Intensity = 1.0f;
        float             Range = 10.0f;  // 有効範囲(ワールド単位)
        bool              IsActive = true;
    };

    struct SpotLightComponent
    {
        DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
        DirectX::XMFLOAT3 Color = { 1.0f,  1.0f, 1.0f };
        float             Intensity = 1.0f;
        float             Range = 10.0f;
        float             InnerConeRad = 0.2f;  // 内円錐角(ラジアン)
        float             OuterConeRad = 0.4f;  // 外円錐角(ラジアン)
        bool              IsActive = true;
    };

}