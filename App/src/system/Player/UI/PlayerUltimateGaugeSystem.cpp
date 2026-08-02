#include "apppch.h"
#include "PlayerUltimateGaugeSystem.h"

#include"PlayerUiTag.h"
#include"FillAmountLerp.h"

#include"../Ultimate/PlayerUltimateComponent.h"
#include<Data/Ultimate/UltimateData.h>
#include<Tag/EntityTag.h>

namespace
{
    constexpr float kFillEpsilon = 0.0001f;
}

void ecs::PlayerUltimateGaugeSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{
    auto playerView = registry.view<PlayerTag, PlayerUltimateComponent>();
    if (playerView.begin() == playerView.end())
    {
        return;
    }

    const auto& ultimate = registry.get<PlayerUltimateComponent>(*playerView.begin());

    // 必要撃破数はマスタデータ、Id=0の単一行から取得する
    const auto* masterData = DATA_MGR(data::UltimateData).GetById(0);
    const int required = (masterData != nullptr) ? masterData->RequiredKillCount : 0;

    // 発動可能または発動中は満タン表示に固定し、それ以外は撃破数の比率で充填する。requiredが0以下なら空扱いにする
    const float ratio = ultimate.IsReady
        ? 1.0f
        : (required > 0
            ? std::clamp(static_cast<float>(ultimate.KillCount) / static_cast<float>(required), 0.0f, 1.0f)
            : 0.0f);

    // 必殺ゲージスプライトへ反映、HPバーと同じ追従ロジック
    auto gaugeView = registry.view<PlayerUltimateGaugeTag, Sprite>();
    gaugeView.each([&](entt::entity e, Sprite& sp)
        {
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
