#include "apppch.h"
#include "HubScene.h"
#include <Utility/config/DebugConfig.h>
#include <ecs/system/manager/ComponentSystemManager.h>
#include <system/GlowAnimation/GlowAnimationComp.h>
#include <system/GlowAnimation/SpriteGlowSystem.h>
#include <system/HubMenu/HubMenuComponent.h>
#include <system/HubMenu/HubMenuInputSystem.h>
#include <system/UI/UiPanelUtility.h>
#include <graphics/Text/Renderer/TextRenderer.h> // ラベルをアイコン中央へ揃えるため幅を実測する
#include "../macros.h"

namespace scene
{
	void HubScene::Initialize()
	{
		// TimeScaleはプロセス全体で共有されシーンを跨いでも持ち越される
		GetTime().SetTimeScale(1.0);

		CreateCompSystem();

		LoadResource();
		CreateBackground();
		CreateOptions();

#if DEV_TOOL_ENABLED
		mPlayerSaveDebugPanel = std::make_unique<debug::PlayerSaveDebugPanel>("HubScene_PlayerSaveDebug");
#endif

		DEBUG_LOG(::sys::eLogLevel::Log, "Hub Scene.");
	}

	void HubScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
		::audio::AudioManager::Get().ClearSceneSounds();

#if DEV_TOOL_ENABLED
		mPlayerSaveDebugPanel.reset();
#endif
	}

	void HubScene::CreateCompSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();

		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::HubMenuInputSystem>(::ecs::eUpdatePhase::PostUpdate);
	}

	void HubScene::LoadResource()
	{
		auto& manager = graphics::TextureManager::Get();
		manager.GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");
		manager.GetOrLoad("Assets/Texture/UI/HubIcon/ui_weapon.png");
		manager.GetOrLoad("Assets/Texture/UI/HubIcon/ui_powerup.png");
	}

	void HubScene::CreateBackground()
	{
		// 専用の背景素材が無いためタイトルと同じ背景を暫定的に流用する
		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920,1080 };
		sprite.Intensity = 1.0f;
		sprite.SetLayer(::ecs::SpriteLayer::Background);

		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 2.5f;
		glow.BaseIntensity = 5;
		glow.Frequency = 0.7f;
		glow.PhaseOffset = 0.0f;

		// 既に共通BGMが再生中ならそのまま継続させ、途切れさせない
		if (!::audio::AudioManager::Get().IsBgmPlaying())
		{
			PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7f);
		}
	}

	void HubScene::CreateOptions()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& window = ::sys::Window::Get();
		auto& textRenderer = ::graphics::TextRenderer::Get();

		auto controllerEntity = manager.CreateEntity();
		manager.AddComponent<::ecs::HubMenuComponent>(controllerEntity);

		// レイアウト定数
		constexpr float kIconWidth = 680.0f;
		constexpr float kIconAspect = 1024.0f / 1536.0f;
		constexpr float kIconHeight = kIconWidth * kIconAspect;
		constexpr float kIconCenterY = 480.0f;
		constexpr float kCardSpacingX = 780.0f;
		constexpr float kLabelGapY = 30.0f;
		constexpr float kLabelTextSize = 34.0f;

		const float centerX = static_cast<float>(window.GetVirtualWidth()) * 0.5f;
		const float labelY = kIconCenterY + kIconHeight * 0.5f + kLabelGapY;

		// 背景ウィンドウ
		{
			constexpr float kWindowPadX = 80.0f;
			constexpr float kWindowPadTop = 40.0f;
			constexpr float kWindowPadBottom = 40.0f;

			const float windowTop = kIconCenterY - kIconHeight * 0.5f - kWindowPadTop;
			const float windowBottom = labelY + kLabelTextSize + kWindowPadBottom;
			const float windowWidth = kCardSpacingX + kIconWidth + kWindowPadX * 2.0f;

			::ecs::uiutil::CreateTranslucentPanel(
				centerX, (windowTop + windowBottom) * 0.5f,
				windowWidth, windowBottom - windowTop,
				0);
		}

		const std::wstring labels[::ecs::HubMenuComponent::kChoiceCount] =
		{
			L"武器・ステージ選択",
			L"ステータス強化",
		};
		const char* iconPaths[::ecs::HubMenuComponent::kChoiceCount] =
		{
			"Assets/Texture/UI/HubIcon/ui_weapon.png",
			"Assets/Texture/UI/HubIcon/ui_powerup.png",
		};

		for (int i = 0; i < ::ecs::HubMenuComponent::kChoiceCount; ++i)
		{
			const float cardCenterX = centerX + (static_cast<float>(i) - 0.5f) * kCardSpacingX;

			// アイコン画像
			{
				auto entity = manager.CreateEntity();
				auto& tr = manager.AddComponent<::ecs::Transform>(entity);
				tr.Set2DPosition(cardCenterX, kIconCenterY);

				auto tex = ::graphics::TextureManager::Get().GetOrLoad(iconPaths[i]);
				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, tex);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { kIconWidth, kIconHeight };
				sprite.SetLayer(::ecs::SpriteLayer::UI, 3);

				registry.emplace<::ecs::HubMenuOptionUiTag>(entity, i);
			}

			// ラベル
			{
				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
				text.Text = labels[i];
				text.Y = labelY;
				text.Size = kLabelTextSize;
				text.Color = (i == 0) ? DirectX::XMFLOAT4{ 1.0f, 0.9f, 0.2f, 1.0f } : DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f };
				text.Layer = 10;

				const float textWidth = textRenderer.MeasureWidth(text.Text, kLabelTextSize);
				text.X = cardCenterX - textWidth * 0.5f;

				registry.emplace<::ecs::HubMenuOptionUiTag>(entity, i);
			}
		}

		// 操作案内
		{
			constexpr float kGuideGapY = 60.0f;
			constexpr float kGuideShiftDownY = 30.0f;
			constexpr float kGuideTextSize = 26.0f;
			constexpr float kGuidePanelWidth = 620.0f;
			constexpr float kGuidePanelPadY = 16.0f;

			const float guideCenterY = labelY + kLabelTextSize + kGuideGapY + kGuideShiftDownY;

			constexpr float kGuidePanelAlpha = 0.85f;

			::ecs::uiutil::CreateTranslucentPanel(
				centerX, guideCenterY,
				kGuidePanelWidth, kGuideTextSize + kGuidePanelPadY * 2.0f,
				0, kGuidePanelAlpha);

			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Y = guideCenterY + textRenderer.MeasureVerticalCenterOffset(kGuideTextSize);
			text.Size = kGuideTextSize;
			text.Color = { 0.6f, 0.6f, 0.6f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::HubGuideUiTag>(entity);
		}
	}

	REGISTER_SCENE_AS(HubScene, HUB_SCENE_NAME);
}