#include "apppch.h"
#include "PlayerHpBarSystem.h"

#include"PlayerUiTag.h"
#include"FillAmountLerp.h"

#include"../Status/PlayerStatusComponent.h"
#include<Tag/EntityTag.h>

namespace
{
    // 逶ｮ讓吶→縺ｮ蟾ｮ縺後％繧梧悴貅縺ｪ繧峨せ繝翫ャ繝励＠縺ｦ霑ｽ蠕薙ｒ謇薙■蛻・ｋ
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

    // HP繝舌・繧ｹ繝励Λ繧､繝医↓驕ｩ蠢・
    auto barView = registry.view<PlayerHpBarTag, Sprite>();
    barView.each([&](entt::entity e, Sprite& sp)
        {
            // FillAmountLerp 縺御ｻ倥＞縺ｦ縺・ｌ縺ｰ貊代ｉ縺九↓霑ｽ蠕薙∫┌縺代ｌ縺ｰ蜊ｳ譎ょ渚譏
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
                    // 螳夐溘〒逶ｮ讓吶∈蟇・○繧具ｼ医が繝ｼ繝舌・繧ｷ繝･繝ｼ繝医＠縺ｪ縺・ｈ縺・け繝ｩ繝ｳ繝暦ｼ・
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
