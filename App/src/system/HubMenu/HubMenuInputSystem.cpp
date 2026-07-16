#include "apppch.h"
#include "HubMenuInputSystem.h"
#include "HubMenuComponent.h"

#include<Scene/Title/TitleScene.h>
#include<Scene/Menu/MenuScene.h>
#include<Scene/StatusUpgrade/StatusUpgradeScene.h>

namespace
{
	const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };
}

namespace ecs
{
	void HubMenuInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto view = registry.view<HubMenuComponent>();
		if (view.begin() == view.end()) return;

		auto& hub = view.get<HubMenuComponent>(*view.begin());
		auto& input = ::sys::InputManager::Get();

		if (input.IsActionPressed("MenuLeft"))
		{
			hub.SelectedIndex = (hub.SelectedIndex + HubMenuComponent::kChoiceCount - 1) % HubMenuComponent::kChoiceCount;
		}
		else if (input.IsActionPressed("MenuRight"))
		{
			hub.SelectedIndex = (hub.SelectedIndex + 1) % HubMenuComponent::kChoiceCount;
		}

		// カーソル位置に応じてハイライトを更新
		registry.view<HubMenuOptionUiTag, TextComponent>().each(
			[&](const HubMenuOptionUiTag& tag, TextComponent& text)
			{
				text.Color = (tag.OptionIndex == hub.SelectedIndex) ? kSelectedColor : kNormalColor;
			});

		if (input.IsActionPressed("Select"))
		{
			PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);

			if (hub.SelectedIndex == 0)
			{
				::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::MenuScene>();
			}
			else
			{
				::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::StatusUpgradeScene>();
			}
			return;
		}

		if (input.IsActionPressed("Cancel"))
		{
			PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
			::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::TitleScene>();
		}
	}
}
