#include "apppch.h"
#include "ResultSystem.h"

#include<Scene/Title/TitleScene.h>
#include<Scene/Game/GameScene.h>
#include<system/Window/Window.h>

namespace ecs
{
	namespace
	{
		// レイアウト定数（個人開発プロトタイプの暫定値。文字幅の厳密な計測はせず概算でセンタリングする）
		constexpr float kTitleTextSize = 64.0f;
		constexpr float kOptionTextSize = 36.0f;
		constexpr float kOptionSpacingX = 200.0f;

		const DirectX::XMFLOAT4 kTitleColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };
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
			// Resultへ入った最初のフレーム：クリア/ゲームオーバーに応じたUIを生成する。
			// 入力受付は次フレームから（生成と同一フレームでの誤入力を避ける）。
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

		auto& manager = ENTITY_MANAGER;
		auto& window = ::sys::Window::Get();
		const float centerX = static_cast<float>(window.GetVirtualWidth()) * 0.5f;
		const float centerY = static_cast<float>(window.GetVirtualHeight()) * 0.5f;

		// タイトル文言(GAME CLEAR / GAME OVER)
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(entity);
			text.Text = (resultType == ::sys::eResultType::Clear) ? L"GAME CLEAR" : L"GAME OVER";
			text.X = centerX - 160.0f;
			text.Y = centerY - 100.0f;
			text.Size = kTitleTextSize;
			text.Color = kTitleColor;
			text.Layer = 10;
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
		}
		else
		{
			// ゲームオーバーはRetry/Titleの2択（先頭カーソルはRetry）
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
			}
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
				::sys::SceneManager::Get().ChangeSceneWithTransition<scene::GameScene>();
			}
			else
			{
				::sys::SceneManager::Get().ChangeSceneWithTransition<scene::TitleScene>();
			}
		}
	}
}
