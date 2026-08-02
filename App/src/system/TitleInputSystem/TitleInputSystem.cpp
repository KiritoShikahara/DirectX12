#include "apppch.h"
#include "TitleInputSystem.h"
#include <Scene/Hub/HubScene.h>
#include <system/Input/InputGuideLabels.h>
#include <graphics/Text/Renderer/TextRenderer.h>

namespace
{
	constexpr float kGuidePanelPadX = 40.0f;
	const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };
}

void sys::TitleInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{
	auto view = registry.view<::ecs::TitleComponent>();
	if (view.begin() == view.end()) return;

	auto& title = view.get<::ecs::TitleComponent>(*view.begin());
	auto& input = ::sys::InputManager::Get();

	if (title.IsConfirmingExit)
	{
		// 十字キー左右どちらでもはい/いいえをトグルする、選択肢は2つだけのため方向は問わない
		if (input.IsActionPressed("MenuLeft") || input.IsActionPressed("MenuRight"))
		{
			title.ConfirmSelectedOption = (title.ConfirmSelectedOption == ecs::eTitleConfirmOption::Yes)
				? ecs::eTitleConfirmOption::No
				: ecs::eTitleConfirmOption::Yes;
		}

		if (input.IsActionPressed("Select"))
		{
			if (title.ConfirmSelectedOption == ecs::eTitleConfirmOption::Yes)
			{
				::sys::Window::Get().RequestQuit();
			}
			else
			{
				title.IsConfirmingExit = false;
				PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
			}
		}
		else if (input.IsActionPressed("Cancel"))
		{
			title.IsConfirmingExit = false;
			PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
		}
	}
	else
	{
		// 決定入力で次の画面へ遷移
		if (input.IsActionPressed("Select"))
		{
			::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::HubScene>();
			PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
		}
		// キャンセル入力でゲーム終了の確認ダイアログを開く、誤操作防止のため選択肢は必ずいいえから始める
		else if (input.IsActionPressed("Cancel"))
		{
			title.IsConfirmingExit = true;
			title.ConfirmSelectedOption = ecs::eTitleConfirmOption::No;
			PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
		}
	}

	const ::sys::eInputDevice device = input.GetLastInputDevice();
	const float screenCenterX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;
	auto& textRenderer = ::graphics::TextRenderer::Get();

	// 操作案内、確認ダイアログ表示中は隠して背後で二重の案内にならないようにする
	std::wstring guideText;
	float guideTextSize = 0.0f;
	registry.view<::ecs::TitleGuideUiTag, ::ecs::TextComponent>().each(
		[&](::ecs::TextComponent& text)
		{
			text.IsVisible = !title.IsConfirmingExit;
			if (title.IsConfirmingExit) return;

			text.Text = std::wstring(::ecs::inputguide::GetSelectLabel(device)) + L":スタート　" +
				::ecs::inputguide::GetCancelLabel(device) + L":終了";

			const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
			text.X = screenCenterX - textWidth * 0.5f;

			guideText = text.Text;
			guideTextSize = text.Size;
		});

	// 操作案内の背景パネル、文言の長さがデバイスによって変わるため毎フレーム幅を合わせ直す
	registry.view<::ecs::TitleGuidePanelUiTag, ::ecs::Sprite>().each(
		[&](::ecs::Sprite& sprite)
		{
			sprite.IsVisible = !title.IsConfirmingExit;
			sprite.Size.x = textRenderer.MeasureWidth(guideText, guideTextSize) + kGuidePanelPadX * 2.0f;
		});

	// 確認ダイアログの背景ウィンドウ、黒半透明で画面中心。IsConfirmingExit中のみ表示する
	registry.view<::ecs::TitleExitConfirmWindowUiTag, ::ecs::Sprite>().each(
		[&](::ecs::Sprite& sprite)
		{
			sprite.IsVisible = title.IsConfirmingExit;
		});

	// 見出し「ゲームを終了しますか？」、内容・位置は固定のため表示切り替えのみ行う
	registry.view<::ecs::TitleExitConfirmQuestionUiTag, ::ecs::TextComponent>().each(
		[&](::ecs::TextComponent& text)
		{
			text.IsVisible = title.IsConfirmingExit;
		});

	// はい/いいえの選択肢、現在選択中の項目だけ黄色にする
	registry.view<::ecs::TitleExitConfirmOptionUiTag, ::ecs::TextComponent>().each(
		[&](const ::ecs::TitleExitConfirmOptionUiTag& tag, ::ecs::TextComponent& text)
		{
			text.IsVisible = title.IsConfirmingExit;
			text.Color = (tag.Option == title.ConfirmSelectedOption) ? kSelectedColor : kNormalColor;
		});

	// 操作案内、十字キーでの選択移動とSelectでの決定を案内する
	registry.view<::ecs::TitleExitConfirmGuideUiTag, ::ecs::TextComponent>().each(
		[&](::ecs::TextComponent& text)
		{
			text.IsVisible = title.IsConfirmingExit;
			if (!title.IsConfirmingExit) return;

			text.Text = std::wstring(::ecs::inputguide::GetMenuMoveLabel(device)) + L":選択　" +
				::ecs::inputguide::GetSelectLabel(device) + L":決定";

			const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
			text.X = screenCenterX - textWidth * 0.5f;
		});
}