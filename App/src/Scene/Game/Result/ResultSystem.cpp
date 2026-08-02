#include "apppch.h"
#include "ResultSystem.h"

#include<Scene/Title/TitleScene.h>
#include<Scene/Game/GameScene.h>
#include<system/Window/Window.h>
#include<system/UI/UiPanelUtility.h>
#include<graphics/Text/Renderer/TextRenderer.h>

#include<limits>
#include<vector>

namespace ecs
{
	namespace
	{
		constexpr float kTitleTextSize = 64.0f;
		constexpr float kOptionTextSize = 36.0f;
		constexpr float kOptionSpacingX = 200.0f;

		const DirectX::XMFLOAT4 kTitleColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };

		constexpr float kResultSeVolume = 0.6f;
	}

	void ResultSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto controllerView = registry.view<GameStateComponent>();
		if (controllerView.begin() == controllerView.end()) return;

		const entt::entity controllerEntity = *controllerView.begin();
		auto& gameState = registry.get<GameStateComponent>(controllerEntity);

		if (gameState.GameState != ::sys::eGameState::Result)
		{
			return;
		}

		auto* result = registry.try_get<ResultComponent>(controllerEntity);
		if (result == nullptr)
		{
			// Resultへ入った最初のフレームでクリア/ゲームオーバーに応じたUIを生成する。入力受付は次フレームから
			EnterResult(registry, controllerEntity, gameState.ResultType);
			return;
		}

		if (gameState.ResultType == ::sys::eResultType::Clear)
		{
			if (::sys::InputManager::Get().IsActionPressed("Select"))
			{
				::sys::SceneManager::Get().ChangeSceneWithTransition<scene::TitleScene>();
			}
		}
		else
		{
			HandleGameOverInput(registry, *result);
		}
	}

	void ResultSystem::EnterResult(entt::registry& registry, entt::entity controllerEntity, ::sys::eResultType resultType)
	{
		registry.emplace<ResultComponent>(controllerEntity);

		if (resultType == ::sys::eResultType::Clear)
		{
			PLAY_SE("Assets/Sound/SE/SE_GameClear.aud", false, kResultSeVolume, false);
		}
		else
		{
			PLAY_SE("Assets/Sound/SE/SE_GameOver.aud", false, kResultSeVolume, false);
		}

		auto& manager = ENTITY_MANAGER;
		auto& window = ::sys::Window::Get();
		auto& textRenderer = ::graphics::TextRenderer::Get();
		const float centerX = static_cast<float>(window.GetVirtualWidth()) * 0.5f;
		const float centerY = static_cast<float>(window.GetVirtualHeight()) * 0.5f;

		// 背景の上に文字が直接乗ると読みづらいため、テキストの実測範囲から黒半透明の板を動的にサイズして敷く
		std::vector<entt::entity> textEntities;

		// タイトル文言、GAME CLEARまたはGAME OVER
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(entity);
			text.Text = (resultType == ::sys::eResultType::Clear) ? L"GAME CLEAR" : L"GAME OVER";
			text.X = centerX - 160.0f;
			text.Y = centerY - 100.0f;
			text.Size = kTitleTextSize;
			text.Color = kTitleColor;
			text.Layer = 10;
			textEntities.push_back(entity);
		}

		if (resultType == ::sys::eResultType::Clear)
		{
			// クリアはRetryの概念が無いため、Select確定のみのプロンプトを出す
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(entity);
			text.Text = L"Select : Title";
			text.X = centerX - 100.0f;
			text.Y = centerY + 20.0f;
			text.Size = kOptionTextSize;
			text.Color = kNormalColor;
			text.Layer = 10;
			textEntities.push_back(entity);
		}
		else
		{
			// ゲームオーバーはRetry/Titleの2択。先頭カーソルはRetry
			const wchar_t* labels[2] = { L"Retry", L"Title" };
			const float offsets[2] = { -kOptionSpacingX - 40.0f, kOptionSpacingX - 40.0f };

			for (int i = 0; i < 2; ++i)
			{
				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<TextComponent>(entity);
				text.Text = labels[i];
				text.X = centerX + offsets[i];
				text.Y = centerY + 20.0f;
				text.Size = kOptionTextSize;
				text.Color = (i == 0) ? kSelectedColor : kNormalColor;
				text.Layer = 10;

				registry.emplace<ResultOptionUiTag>(entity, i);
				textEntities.push_back(entity);
			}
		}

		{
			float minX = (std::numeric_limits<float>::max)();
			float maxX = (std::numeric_limits<float>::lowest)();
			float minY = (std::numeric_limits<float>::max)();
			float maxY = (std::numeric_limits<float>::lowest)();

			for (entt::entity entity : textEntities)
			{
				const auto& text = registry.get<TextComponent>(entity);
				minX = std::min(minX, text.X);
				maxX = std::max(maxX, text.X + textRenderer.MeasureWidth(text.Text, text.Size));
				minY = std::min(minY, text.Y);
				maxY = std::max(maxY, text.Y + text.Size);
			}

			constexpr float kPanelPadX = 80.0f;
			constexpr float kPanelPadTop = 50.0f;
			constexpr float kPanelPadBottom = 50.0f;

			const float left = minX - kPanelPadX;
			const float right = maxX + kPanelPadX;
			const float top = minY - kPanelPadTop;
			const float bottom = maxY + kPanelPadBottom;

			::ecs::uiutil::CreateTranslucentPanel(
				(left + right) * 0.5f, (top + bottom) * 0.5f,
				right - left, bottom - top,
				0);
		}
	}

	void ResultSystem::HandleGameOverInput(entt::registry& registry, ResultComponent& result)
	{
		auto& input = ::sys::InputManager::Get();

		// Retry/Titleの2択なのでどちらのキーでもトグルする
		if (input.IsActionPressed("MenuLeft") || input.IsActionPressed("MenuRight"))
		{
			result.SelectedIndex = 1 - result.SelectedIndex;
		}

		registry.view<ResultOptionUiTag, TextComponent>().each(
			[&](const ResultOptionUiTag& tag, TextComponent& text)
			{
				text.Color = (tag.OptionIndex == result.SelectedIndex) ? kSelectedColor : kNormalColor;
			});

		if (input.IsActionPressed("Select"))
		{
			if (result.SelectedIndex == 0)
			{
				// TODO: LoadingScene経由の非同期先読みは原因未特定のクラッシュが再現したため見送り、同期読み込み方式に戻す
				::sys::SceneManager::Get().ChangeSceneWithTransition<scene::GameScene>();
			}
			else
			{
				::sys::SceneManager::Get().ChangeSceneWithTransition<scene::TitleScene>();
			}
		}
	}
}
