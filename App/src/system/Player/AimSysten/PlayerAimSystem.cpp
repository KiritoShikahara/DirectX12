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

    /// <summary>
    /// 右スティックの傾きから攻撃方向を求める。
    /// </summary>
    bool PlayerAimSystem::TryGetPadAimDirection(XMFLOAT3& outDirection)
    {
        // GetRightStick3D() はデッドゾーン適用済み。
        // 傾いていなければ {0, 0} が返るため、そのまま長さで判定できる。
        // 3D 系のため y が前方（+Z）に対応する。
        const XMFLOAT2 stick = InputManager::Get().GetPadManager()->GetRightStick3D();

        const XMVECTOR vec = XMVectorSet(stick.x, 0.0f, stick.y, 0.0f);

        if (XMVectorGetX(XMVector3LengthSq(vec)) < kMinDirectionLengthSq)
        {
            return false;
        }

        XMStoreFloat3(&outDirection, XMVector3Normalize(vec));
        return true;
    }

    /// <summary>
    /// マウスカーソルの位置から攻撃方向を求める。
    /// </summary>
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

        // マウス座標は仮想解像度基準に変換して渡す
        // （ScreenPointToRay 系は仮想解像度を前提とするため）
        const XMFLOAT2 mousePos = InputManager::Get().GetMouseVirtualPosition();

        // プレイヤーと同じ高さの水平面との交点を求める
        XMFLOAT3 cursorWorldPos = {};
        if (!cameraSystem.ScreenPointToWorldOnPlaneY(
            registry, mousePos, playerPosition.y, cursorWorldPos))
        {
            return false;
        }

        // カーソル位置 - プレイヤー位置（水平成分のみ）
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