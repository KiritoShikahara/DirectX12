#include "apppch.h"
#include "TitleScene.h"
#include <ecs/system/manager/ComponentSystemManager.h>
#include <system/GlowAnimation/GlowAnimationComp.h>
#include <system/GlowAnimation/SpriteGlowSystem.h>
#include <system/TitleInputSystem/TitleInputSystem.h>
#include <system/UI/UiPanelUtility.h>
#include <graphics/Text/Renderer/TextRenderer.h>
#include "../macros.h"

namespace scene
{
	void TitleScene::Initialize()
	{
		// タイムスケールを確実に1.0に戻す
		GetTime().SetTimeScale(1.0);

		CreateCompSystem();

		LoadResource();
		CreateBackground();
		CreateLogo();
		CreatePromptText();
		CreateControlGuide();
		CreateExitConfirmUi();

		DEBUG_LOG(::sys::eLogLevel::Log, "Title Scene.");
	}

	void TitleScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
		::audio::AudioManager::Get().ClearSceneSounds();
	}

	void TitleScene::CreateCompSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();

		// グロー演出
		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);

		// 入力
		manager.AddUserSystem<::sys::TitleInputSystem>(::ecs::eUpdatePhase::PostUpdate);
	}

	void TitleScene::LoadResource()
	{
		{
			auto& manager = graphics::TextureManager::Get();
			manager.GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");
			manager.GetOrLoad("Assets/Texture/Title/TX_Logo.png");
		}
	}

	void TitleScene::CreateBackground()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920, 1080 };
		sprite.Intensity = 1.0f;
		sprite.SetLayer(::ecs::SpriteLayer::Background);

		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 2.5f;
		glow.BaseIntensity = 5;
		glow.Frequency = 0.7;
		glow.PhaseOffset = 0.0f;

		// 既に共通BGMが再生中ならそのまま継続させ、途切れさせない
		if (!::audio::AudioManager::Get().IsBgmPlaying())
		{
			PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7);
		}
	}

	void TitleScene::CreateLogo()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& window = ::sys::Window::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_Logo.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		trans.Set2DPosition(window.GetVirtualWidth() / 2, window.GetVirtualHeight() / 5 * 2);

		float scale = 0.8f;
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920, 1080 };
		sprite.DrawScale = { scale, scale };
		sprite.Intensity = 1.0f;
		sprite.Pivot = { 0.5, 0.5 };
		sprite.SetLayer(::ecs::SpriteLayer::Character);

		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 1.0;
		glow.BaseIntensity = 2;
		glow.Frequency = 0.7;
		glow.PhaseOffset = 0.1;
	}

	void TitleScene::CreatePromptText()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& window = ::sys::Window::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_Prompt.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		trans.Set2DPosition(window.GetVirtualWidth() / 2, window.GetVirtualHeight() / 5 * 4);

		float scale = 0.6f;
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920, 1080 };
		sprite.DrawScale = { scale, scale };
		sprite.Intensity = 1.0f;
		sprite.Pivot = { 0.5, 0.5 };
		sprite.SetLayer(::ecs::SpriteLayer::Character);

		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 0.5;
		glow.BaseIntensity = 1.2;
		glow.Frequency = 1.5;
		glow.PhaseOffset = 0;
	}

	void TitleScene::CreateControlGuide()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& window = ::sys::Window::Get();

		auto& textRenderer = ::graphics::TextRenderer::Get();

		// 操作ガイドの表示位置とテキストサイズ
		const float guideCenterY = static_cast<float>(window.GetVirtualHeight()) / 5.0f * 4.0f + 90.0f;
		constexpr float kTextSize = 30.0f;

		// 背景パネルの生成、幅は文言の長さに応じてTitleInputSystemが毎フレーム合わせ直す
		constexpr float kGuidePanelWidth = 260.0f;
		constexpr float kGuidePanelPadY = 16.0f;
		auto guidePanel = ::ecs::uiutil::CreateTranslucentPanel(
			static_cast<float>(window.GetVirtualWidth()) * 0.5f,
			guideCenterY,
			kGuidePanelWidth,
			kTextSize + kGuidePanelPadY * 2.0f,
			0);
		registry.emplace<::ecs::TitleGuidePanelUiTag>(guidePanel);

		auto entity = manager.CreateEntity();
		auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
		text.Y = guideCenterY + textRenderer.MeasureVerticalCenterOffset(kTextSize);
		text.Size = kTextSize;
		text.Color = { 0.85f, 0.9f, 1.0f, 1.0f };
		text.Layer = 10;

		registry.emplace<::ecs::TitleGuideUiTag>(entity);
	}

	void TitleScene::CreateExitConfirmUi()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& window = ::sys::Window::Get();
		auto& textRenderer = ::graphics::TextRenderer::Get();

		const float centerX = static_cast<float>(window.GetVirtualWidth()) * 0.5f;
		const float centerY = static_cast<float>(window.GetVirtualHeight()) * 0.5f;

		// ダイアログの表示状態を保持するコントローラーエンティティ
		auto controllerEntity = manager.CreateEntity();
		manager.AddComponent<::ecs::TitleComponent>(controllerEntity);

		// 背景ウィンドウ、黒半透明で画面中心。IsConfirmingExit中のみTitleInputSystemが表示する
		constexpr float kConfirmWindowWidth = 700.0f;
		constexpr float kConfirmWindowHeight = 220.0f;
		{
			auto entity = ::ecs::uiutil::CreateTranslucentPanel(
				centerX, centerY,
				kConfirmWindowWidth, kConfirmWindowHeight,
				10,
				1.0f);
			registry.get<::ecs::Sprite>(entity).IsVisible = false;

			registry.emplace<::ecs::TitleExitConfirmWindowUiTag>(entity);
		}

		// 見出し、内容・位置は固定のためここで確定させる
		constexpr float kQuestionOffsetY = -60.0f;
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = L"ゲームを終了しますか？";
			text.Size = 28.0f;
			text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			text.Layer = 20;
			text.IsVisible = false;

			const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
			text.X = centerX - textWidth * 0.5f;
			text.Y = centerY + kQuestionOffsetY;

			registry.emplace<::ecs::TitleExitConfirmQuestionUiTag>(entity);
		}

		// はい/いいえの選択肢、内容・位置は固定で表示色のみTitleInputSystemが毎フレーム更新する
		constexpr float kOptionOffsetX = 90.0f;
		constexpr float kOptionTextSize = 30.0f;
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = L"はい";
			text.Size = kOptionTextSize;
			text.Layer = 20;
			text.IsVisible = false;

			const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
			text.X = centerX - kOptionOffsetX - textWidth * 0.5f;
			text.Y = centerY;

			registry.emplace<::ecs::TitleExitConfirmOptionUiTag>(entity, ::ecs::TitleExitConfirmOptionUiTag{ ::ecs::eTitleConfirmOption::Yes });
		}
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = L"いいえ";
			text.Size = kOptionTextSize;
			text.Layer = 20;
			text.IsVisible = false;

			const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
			text.X = centerX + kOptionOffsetX - textWidth * 0.5f;
			text.Y = centerY;

			registry.emplace<::ecs::TitleExitConfirmOptionUiTag>(entity, ::ecs::TitleExitConfirmOptionUiTag{ ::ecs::eTitleConfirmOption::No });
		}

		// 操作案内、内容はデバイスに応じてTitleInputSystemが毎フレーム更新する
		constexpr float kGuideOffsetY = 55.0f;
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Y = centerY + kGuideOffsetY;
			text.Size = 24.0f;
			text.Color = { 0.7f, 0.7f, 0.7f, 1.0f };
			text.Layer = 20;
			text.IsVisible = false;

			registry.emplace<::ecs::TitleExitConfirmGuideUiTag>(entity);
		}
	}

	REGISTER_SCENE_AS(TitleScene, TITLE_SCENE_NAME);
}