#include "apppch.h"
#include "WaveTimerUiSystem.h"

#include"WaveComponent.h"
#include"WaveTimerUiTag.h"

#include<format>
#include<algorithm>
#include<cmath>

namespace ecs
{
	void WaveTimerUiSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto waveView = registry.view<::ecs::WaveComponent>();
		if (waveView.begin() == waveView.end()) return;

		const auto& wave = registry.get<::ecs::WaveComponent>(*waveView.begin());
		const float remaining = std::max(0.0f, wave.ClearTime - wave.ElapsedTime);
		const int totalSeconds = static_cast<int>(std::ceil(remaining));
		const int minutes = totalSeconds / 60;
		const int seconds = totalSeconds % 60;

		registry.view<::ecs::WaveTimerUiTag, ::ecs::TextComponent>().each(
			[&](ecs::TextComponent& text)
			{
				text.Text = std::format(L"{:02}:{:02}", minutes, seconds);
			});
	}
}
