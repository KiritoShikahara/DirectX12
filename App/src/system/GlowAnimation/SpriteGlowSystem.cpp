#include "apppch.h"
#include "SpriteGlowSystem.h"
#include"GlowAnimationComp.h"

void ecs::SpriteGlowSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{
	// タイトルは止めることないのでraw
	mTotalTime += rawDeltaTime;

    auto view = registry.view<ecs::Sprite, ecs::GlowAnimation>();

    view.each([&](auto entity, ecs::Sprite& sp, const ecs::GlowAnimation& glow)
        {
            // サイン波による光度の計算
            // Formula: Base + Amplitude * sin(Time * Frequency + Offset)
            sp.Intensity = glow.BaseIntensity + glow.Amplitude * std::sin(mTotalTime * glow.Frequency + glow.PhaseOffset);
        });

}
