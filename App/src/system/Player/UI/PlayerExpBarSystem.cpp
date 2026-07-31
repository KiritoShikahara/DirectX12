#include "apppch.h"
#include "PlayerExpBarSystem.h"

#include"PlayerUiTag.h"
#include"FillAmountLerp.h"

#include"../Level/PlayerLevelComponent.h"
#include<Tag/EntityTag.h>

namespace
{
	// 目標との差がこれ未満ならスナップして追従を打ち切る(PlayerHpBarSystemと同値)
	constexpr float kFillEpsilon = 0.0001f;
}

void ecs::PlayerExpBarSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{
	auto playerView = registry.view<PlayerTag, PlayerLevelComponent>();
	if (playerView.begin() == playerView.end())
	{
		return;
	}

	const auto& level = registry.get<PlayerLevelComponent>(*playerView.begin());

	const float ratio = (level.ExperienceToNextLevel > 0.0f)
		? std::clamp(level.Experience / level.ExperienceToNextLevel, 0.0f, 1.0f)
		: 0.0f;

	// 経験値バースプライトへ反映(HPバー・必殺ゲージと同じ追従ロジック)
	auto barView = registry.view<PlayerExpBarTag, Sprite>();
	barView.each([&](entt::entity e, Sprite& sp)
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
					const float step = lerp->Speed * deltaTime;
					sp.FillAmount += std::clamp(diff, -step, step);
				}
			}
			else
			{
				sp.FillAmount = ratio;
			}
		});

	// レベル表示テキスト
	registry.view<PlayerLevelTextTag, TextComponent>().each(
		[&](TextComponent& text)
		{
			text.Text = L"Lv." + std::to_wstring(level.Level);
		});
}
