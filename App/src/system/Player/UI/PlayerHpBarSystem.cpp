#include "apppch.h"
#include "PlayerHpBarSystem.h"

#include"PlayerUiTag.h"
#include"FillAmountLerp.h"

#include"../Status/PlayerStatusComponent.h"
#include<Tag/EntityTag.h>

namespace
{
    constexpr float kFillEpsilon = 0.0001f;
}

void ecs::PlayerHpBarSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{
    auto playerView = registry.view<PlayerTag, PlayerStatusComponent>();
    if (playerView.size_hint() == 0)
    {
        return;
    }

    const auto& status = registry.get<PlayerStatusComponent>(*playerView.begin());

    const float ratio = (status.Current.MaxHp > 0.0f)
        ? std::clamp(status.CurrentHp / status.Current.MaxHp, 0.0f, 1.0f)
        : 0.0f;

    // HPバースプライトに適用
    auto barView = registry.view<PlayerHpBarTag, Sprite>();
    barView.each([&](entt::entity e, Sprite& sp)
        {
            // FillAmountLerpが付いていれば滑らかに追従、無ければ即時反映
            if (auto* lerp = registry.try_get<FillAmountLerp>(e))
            {
                lerp->SetTarget(ratio);

                const float diff = lerp->Target - sp.FillAmount;
                if (std::abs(diff) < kFillEpsilon)
                {
                    sp.FillAmount = lerp->Target;
                }
                else
                {
                    // 定速で目標へ寄せる、オーバーシュートしないようクランプ
                    const float step = lerp->Speed * deltaTime;
                    sp.FillAmount += std::clamp(diff, -step, step);
                }
            }
            else
            {
                sp.FillAmount = ratio;
            }
        });
}
