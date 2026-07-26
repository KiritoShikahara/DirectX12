#include "apppch.h"
#include "HubMenuInputSystem.h"
#include "HubMenuComponent.h"

#include<Scene/Title/TitleScene.h>
#include<Scene/Menu/MenuScene.h>
#include<Scene/StatusUpgrade/StatusUpgradeScene.h>
#include<system/Input/InputGuideLabels.h>
#include<graphics/Text/Renderer/TextRenderer.h> // 操作案内テキストの水平中央揃えに使う

namespace
{
	const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };

	// 選択中カードのアイコンを明るくして目立たせる(通常時は等倍)
	constexpr float kNormalIconIntensity = 1.0f;
	constexpr float kSelectedIconIntensity = 1.4f;
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

		// カーソル位置に応じてハイライトを更新(ラベル色 + アイコンの明るさ)
		registry.view<HubMenuOptionUiTag, TextComponent>().each(
			[&](const HubMenuOptionUiTag& tag, TextComponent& text)
			{
				text.Color = (tag.OptionIndex == hub.SelectedIndex) ? kSelectedColor : kNormalColor;
			});

		registry.view<HubMenuOptionUiTag, Sprite>().each(
			[&](const HubMenuOptionUiTag& tag, Sprite& sprite)
			{
				sprite.Intensity = (tag.OptionIndex == hub.SelectedIndex) ? kSelectedIconIntensity : kNormalIconIntensity;
			});

		// 操作案内。最後に使われた入力デバイスに応じてボタン表示名を切り替える
		{
			const ::sys::eInputDevice device = input.GetLastInputDevice();
			const float screenCenterX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;
			auto& textRenderer = ::graphics::TextRenderer::Get();

			registry.view<HubGuideUiTag, TextComponent>().each(
				[&](TextComponent& text)
				{
					text.Text = std::wstring(ecs::inputguide::GetMenuMoveLabel(device)) + L":選択　" +
						ecs::inputguide::GetSelectLabel(device) + L":決定　" +
						ecs::inputguide::GetCancelLabel(device) + L":戻る";

					const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
					text.X = screenCenterX - textWidth * 0.5f;
				});
		}

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
