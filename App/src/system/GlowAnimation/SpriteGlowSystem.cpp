#include "apppch.h"
#include "SpriteGlowSystem.h"
#include"GlowAnimationComp.h"

void ecs::SpriteGlowSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{
	// 繧ｿ繧､繝医Ν縺ｯ豁｢繧√ｋ縺薙→縺ｪ縺・・縺ｧraw
	mTotalTime += rawDeltaTime;

    auto view = registry.view<ecs::Sprite, ecs::GlowAnimation>();

    view.each([&](auto entity, ecs::Sprite& sp, const ecs::GlowAnimation& glow)
        {
            // 繧ｵ繧､繝ｳ豕｢縺ｫ繧医ｋ蜈牙ｺｦ縺ｮ險育ｮ・
            // Formula: Base + Amplitude * sin(Time * Frequency + Offset)
            sp.Intensity = glow.BaseIntensity + glow.Amplitude * std::sin(mTotalTime * glow.Frequency + glow.PhaseOffset);
        });

}
