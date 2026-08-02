#include"apppch.h"
#include"PlayerAimSystem.h"

#include<ecs/component/transform/TransformComponent.h>
#include<system/Input/InputManager.h>
#include<system/Camera/CameraSystem.h>

#include"PlayerAimComponent.h"

using namespace DirectX;

namespace sys
{
    void PlayerAimSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        const eInputDevice device = InputManager::Get().GetLastInputDevice();

        registry.view<ecs::Transform, ecs::PlayerAimComponent>().each(
            [&](ecs::Transform& transform, ecs::PlayerAimComponent& aim)
            {
                XMFLOAT3 direction = {};
                bool     isValid = false;

                switch (device)
                {
                case eInputDevice::Pad:
                    isValid = TryGetPadAimDirection(direction);
                    break;

                case eInputDevice::KeyboardMouse:
                    isValid = TryGetMouseAimDirection(
                        registry, transform.GetPosition(), direction);
                    break;

                default:
                    break;
                }

                // 方向が確定しなかったフレームは直前の値を維持する
                if (isValid)
                {
                    aim.Direction = direction;
                }
            });
    }

    bool PlayerAimSystem::TryGetPadAimDirection(XMFLOAT3& outDirection)
    {
        // GetRightStick3Dはデッドゾーン適用済み。3D系のためyが前方Zに対応する
        const XMFLOAT2 stick = InputManager::Get().GetPadManager()->GetRightStick3D();

        const XMVECTOR vec = XMVectorSet(stick.x, 0.0f, stick.y, 0.0f);

        if (XMVectorGetX(XMVector3LengthSq(vec)) < kMinDirectionLengthSq)
        {
            return false;
        }

        XMStoreFloat3(&outDirection, XMVector3Normalize(vec));
        return true;
    }

    bool PlayerAimSystem::TryGetMouseAimDirection(
        entt::registry& registry,
        const XMFLOAT3& playerPosition,
        XMFLOAT3& outDirection)
    {
        auto& cameraSystem = CameraSystem::Get();
        if (!cameraSystem.HasMainCamera())
        {
            return false;
        }

        // マウス座標を仮想解像度基準に変換して渡す
        const XMFLOAT2 mousePos = InputManager::Get().GetMouseVirtualPosition();

        // プレイヤーと同じ高さの水平面との交点を求める
        XMFLOAT3 cursorWorldPos = {};
        if (!cameraSystem.ScreenPointToWorldOnPlaneY(
            registry, mousePos, playerPosition.y, cursorWorldPos))
        {
            return false;
        }

        // カーソル位置とプレイヤー位置の差、水平成分のみ
        const XMVECTOR vec = XMVectorSet(
            cursorWorldPos.x - playerPosition.x,
            0.0f,
            cursorWorldPos.z - playerPosition.z,
            0.0f);

        // カーソルがプレイヤーとほぼ重なっている場合は方向が定まらない
        if (XMVectorGetX(XMVector3LengthSq(vec)) < kMinDirectionLengthSq)
        {
            return false;
        }

        XMStoreFloat3(&outDirection, XMVector3Normalize(vec));
        return true;
    }
}
