#include "apppch.h"
#include "DamageNumberSystem.h"

#include"DamageNumberComponent.h"
#include<system/Camera/CameraSystem.h>
#include<system/Window/Window.h>

namespace ecs
{
    void DamageNumberSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        auto& cameraSys = ::sys::CameraSystem::Get();
        if (!cameraSys.HasMainCamera()) return;

        const auto* camera = registry.try_get<ecs::CameraComponent>(cameraSys.GetMainCameraEntity());
        if (camera == nullptr) return;

        auto& window = ::sys::Window::Get();
        const float screenWidth = static_cast<float>(window.GetVirtualWidth());
        const float screenHeight = static_cast<float>(window.GetVirtualHeight());
        const DirectX::XMMATRIX viewProj = camera->GetViewProjectionMatrix();

        mExpired.clear();

        registry.view<ecs::DamageNumberComponent, ecs::TextComponent>().each(
            [&](entt::entity entity, ecs::DamageNumberComponent& number, ecs::TextComponent& text)
            {
                number.WorldPosition.y += number.RiseSpeed * deltaTime;
                number.RemainingTime -= deltaTime;

                if (number.RemainingTime <= 0.0f)
                {
                    mExpired.push_back(entity);
                    return;
                }

                using namespace DirectX;

                const XMVECTOR worldPos = XMLoadFloat3(&number.WorldPosition);
                const XMVECTOR ndc = XMVector3TransformCoord(worldPos, viewProj);

                XMFLOAT3 ndcResult;
                XMStoreFloat3(&ndcResult, ndc);

                // NDC([-1,1], Y上向き) からスクリーン座標(左上原点、Y下向き)へ変換する
                text.X = (ndcResult.x * 0.5f + 0.5f) * screenWidth;
                text.Y = (1.0f - (ndcResult.y * 0.5f + 0.5f)) * screenHeight;

                // 残り時間の割合でフェードアウトする
                const float alpha = std::clamp(number.RemainingTime / number.TotalTime, 0.0f, 1.0f);
                text.Color.w = alpha;
            });

        for (entt::entity entity : mExpired)
        {
            registry.destroy(entity);
        }
    }
}
