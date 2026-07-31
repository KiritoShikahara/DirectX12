#include "apppch.h"
#include "TitleInputSystem.h"
#include <Scene/Hub/HubScene.h>
#include <system/Input/InputGuideLabels.h>
#include <graphics/Text/Renderer/TextRenderer.h> // 操作案内テキストの水平中央揃えに使う

void sys::TitleInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{

	auto& input = ::sys::InputManager::Get();

	// セレクトなら SE + ハブ画面遷移（武器・ステージ選択/ステータス強化の2択はHubSceneが担当）
	if (input.IsActionPressed("Select") == true)
	{
		::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::HubScene>();
		PLAY_SE("Assets/Sound/SE/SE_Select.aud",false,1,false);
	}

	// 終了なら SE + 確認画面

	// 終了のシステムは全画面で共通できるように別システムとして存在。
	// 専用のコンポーネントの存在で処理分岐をするようにする。

	// 操作案内(「PUSH TO START」画像の下のボタン名)。最後に使われた入力デバイスに応じて更新する
	const ::sys::eInputDevice device = input.GetLastInputDevice();
	const float screenCenterX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;
	auto& textRenderer = ::graphics::TextRenderer::Get();
	registry.view<::ecs::TitleGuideUiTag, ::ecs::TextComponent>().each(
		[device, screenCenterX, &textRenderer](::ecs::TextComponent& text)
		{
			text.Text = ::ecs::inputguide::GetSelectLabel(device);
			const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
			text.X = screenCenterX - textWidth * 0.5f;
		});
}
