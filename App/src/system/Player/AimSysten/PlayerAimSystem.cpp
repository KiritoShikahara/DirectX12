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

                // 譁ｹ蜷代′遒ｺ螳壹＠縺ｪ縺九▲縺溘ヵ繝ｬ繝ｼ繝縺ｯ逶ｴ蜑阪・蛟､繧堤ｶｭ謖√☆繧・
                if (isValid)
                {
                    aim.Direction = direction;
                }
            });
    }

    /// <summary>
    /// 蜿ｳ繧ｹ繝・ぅ繝・け縺ｮ蛯ｾ縺阪°繧画判謦・婿蜷代ｒ豎ゅａ繧九・
    /// </summary>
    bool PlayerAimSystem::TryGetPadAimDirection(XMFLOAT3& outDirection)
    {
        // GetRightStick3D() 縺ｯ繝・ャ繝峨だ繝ｼ繝ｳ驕ｩ逕ｨ貂医∩縲・
        // 蛯ｾ縺・※縺・↑縺代ｌ縺ｰ {0, 0} 縺瑚ｿ斐ｋ縺溘ａ縲√◎縺ｮ縺ｾ縺ｾ髟ｷ縺輔〒蛻､螳壹〒縺阪ｋ縲・
        // 3D 邉ｻ縺ｮ縺溘ａ y 縺悟燕譁ｹ・・Z・峨↓蟇ｾ蠢懊☆繧九・
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
    /// 繝槭え繧ｹ繧ｫ繝ｼ繧ｽ繝ｫ縺ｮ菴咲ｽｮ縺九ｉ謾ｻ謦・婿蜷代ｒ豎ゅａ繧九・
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

        // 繝槭え繧ｹ蠎ｧ讓吶・莉ｮ諠ｳ隗｣蜒丞ｺｦ蝓ｺ貅悶↓螟画鋤縺励※貂｡縺・
        // ・・creenPointToRay 邉ｻ縺ｯ莉ｮ諠ｳ隗｣蜒丞ｺｦ繧貞燕謠舌→縺吶ｋ縺溘ａ・・
        const XMFLOAT2 mousePos = InputManager::Get().GetMouseVirtualPosition();

        // 繝励Ξ繧､繝､繝ｼ縺ｨ蜷後§鬮倥＆縺ｮ豌ｴ蟷ｳ髱｢縺ｨ縺ｮ莠､轤ｹ繧呈ｱゅａ繧・
        XMFLOAT3 cursorWorldPos = {};
        if (!cameraSystem.ScreenPointToWorldOnPlaneY(
            registry, mousePos, playerPosition.y, cursorWorldPos))
        {
            return false;
        }

        // 繧ｫ繝ｼ繧ｽ繝ｫ菴咲ｽｮ - 繝励Ξ繧､繝､繝ｼ菴咲ｽｮ・域ｰｴ蟷ｳ謌仙・縺ｮ縺ｿ・・
        const XMVECTOR vec = XMVectorSet(
            cursorWorldPos.x - playerPosition.x,
            0.0f,
            cursorWorldPos.z - playerPosition.z,
            0.0f);

        // 繧ｫ繝ｼ繧ｽ繝ｫ縺後・繝ｬ繧､繝､繝ｼ縺ｨ縺ｻ縺ｼ驥阪↑縺｣縺ｦ縺・ｋ蝣ｴ蜷医・譁ｹ蜷代′螳壹∪繧峨↑縺・
        if (XMVectorGetX(XMVector3LengthSq(vec)) < kMinDirectionLengthSq)
        {
            return false;
        }

        XMStoreFloat3(&outDirection, XMVector3Normalize(vec));
        return true;
    }
}