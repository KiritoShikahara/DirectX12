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
                // ダメージ数値を上方向へ移動
                number.WorldPosition.y += number.RiseSpeed * deltaTime;

                // 残り表示時間を更新
                number.RemainingTime -= deltaTime;

                // 表示時間が終了したら削除対象に追加
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

                // ワールド座標をスクリーン座標へ変換
                text.X = (ndcResult.x * 0.5f + 0.5f) * screenWidth;
                text.Y = (1.0f - (ndcResult.y * 0.5f + 0.5f)) * screenHeight;

                // 残り時間に応じて透明度を更新
                const float alpha = std::clamp(number.RemainingTime / number.TotalTime, 0.0f, 1.0f);
                text.Color.w = alpha;
            });

        // 表示が終了したエンティティを削除
        for (entt::entity entity : mExpired)
        {
            registry.destroy(entity);
        }
    }
}