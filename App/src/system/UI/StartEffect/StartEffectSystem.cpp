#include "apppch.h"
#include "StartEffectSystem.h"

#include"StartEffectComponent.h"

namespace ecs
{
	void StartEffectSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		mExpired.clear();

		registry.view<StartEffectComponent, TextComponent>().each(
			[&](entt::entity entity, StartEffectComponent& effect, TextComponent& text)
			{
				effect.Elapsed += deltaTime;

				const float fadeInEnd = effect.FadeInDuration;
				const float holdEnd = fadeInEnd + effect.HoldDuration;
				const float totalDuration = holdEnd + effect.FadeOutDuration;

				float alpha;
				if (effect.Elapsed < fadeInEnd)
				{
					alpha = (effect.FadeInDuration > 0.0f)
						? (effect.Elapsed / effect.FadeInDuration) : 1.0f;
				}
				else if (effect.Elapsed < holdEnd)
				{
					alpha = 1.0f;
				}
				else if (effect.Elapsed < totalDuration)
				{
					const float t = (effect.FadeOutDuration > 0.0f)
						? (effect.Elapsed - holdEnd) / effect.FadeOutDuration : 1.0f;
					alpha = 1.0f - std::clamp(t, 0.0f, 1.0f);
				}
				else
				{
					mExpired.push_back(entity);
					return;
				}

				text.Color.w = std::clamp(alpha, 0.0f, 1.0f);
			});

		for (entt::entity entity : mExpired)
		{
			registry.destroy(entity);
		}
	}
}
