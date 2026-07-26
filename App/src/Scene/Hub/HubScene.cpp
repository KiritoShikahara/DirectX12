#include "apppch.h"
#include "HubScene.h"
#include<ecs/system/manager/ComponentSystemManager.h>

#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/HubMenu/HubMenuComponent.h>
#include<system/HubMenu/HubMenuInputSystem.h>
#include<system/UI/UiPanelUtility.h>
#include<graphics/Text/Renderer/TextRenderer.h> // ラベルをアイコン中央へ揃えるため、幅を実測する

#include"../macros.h"

namespace scene
{
	void HubScene::Initialize()
	{
		// TimeScaleはプロセス全体で共有され、シーンを跨いでも持ち越される。
		// GameOver/PerkSelect等でTimeScale=0.0のままResult→Titleへ遷移してくるケースがあるため、
		// 一時停止の概念が無いHubでは必ず1.0へ戻す（TitleScene/MenuSceneと同じ理由）。
		GetTime().SetTimeScale(1.0);

		CreateCompSystem();

		LoadResource();
		CreateBackground();
		CreateOptions();

		DEBUG_LOG(::sys::eLogLevel::Log, "Hub Scene.");
	}

	void HubScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
		::audio::AudioManager::Get().ClearSceneSounds();
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
		// 専用の背景素材が無いため、タイトルと同じ背景を暫定的に流用する
		// （専用素材が用意でき次第、Assets/Texture/Hub/配下へ差し替える）。
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

		PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7f);
	}

	void HubScene::CreateOptions()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& window = ::sys::Window::Get();
		auto& textRenderer = ::graphics::TextRenderer::Get();

		auto controllerEntity = manager.CreateEntity();
		manager.AddComponent<::ecs::HubMenuComponent>(controllerEntity);

		// レイアウト定数（仮想解像度1920x1080基準、画面中央に2択を横並び。個人開発プロトタイプの暫定値）
		// カード = アイコン画像(上) + ラベル(下)。元画像(ui_weapon.png/ui_powerup.png)は1536x1024(3:2)。
		constexpr float kIconWidth = 680.0f;
		constexpr float kIconAspect = 1024.0f / 1536.0f; // 元画像のアスペクト比を維持する
		constexpr float kIconHeight = kIconWidth * kIconAspect;
		constexpr float kIconCenterY = 480.0f;
		constexpr float kCardSpacingX = 780.0f; // カード中心どうしの横間隔(アイコン幅+隙間分)
		constexpr float kLabelGapY = 30.0f;     // アイコン下端とラベルの間隔
		constexpr float kLabelTextSize = 34.0f;

		const float centerX = static_cast<float>(window.GetVirtualWidth()) * 0.5f;
		const float labelY = kIconCenterY + kIconHeight * 0.5f + kLabelGapY;

		// 背景(タイトル画像流用)の上に直接乗ると読みづらいため、カード全体を覆う黒半透明の
		// ウィンドウを敷く(StatusUpgradeSceneの読みやすさ用ウィンドウと同じ手法:
		// 白テクスチャをColorで黒+半透明に着色したSpriteで代用する)。
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
				0); // アイコン(offset3)より奥
		}

		const std::wstring labels[::ecs::HubMenuComponent::kChoiceCount] =
		{
			L"武器・ステージ選択",
			L"ステータス強化",
		};
		// 専用アイコン未作成の場合はloading.pngで代用する方針(WeaponIconRegistry等と同じ)だが、
		// 本画面は2件固定でHubIconフォルダに専用素材が用意済みのためそのまま使う
		const char* iconPaths[::ecs::HubMenuComponent::kChoiceCount] =
		{
			"Assets/Texture/UI/HubIcon/ui_weapon.png",
			"Assets/Texture/UI/HubIcon/ui_powerup.png",
		};

		for (int i = 0; i < ::ecs::HubMenuComponent::kChoiceCount; ++i)
		{
			const float cardCenterX = centerX + (static_cast<float>(i) - 0.5f) * kCardSpacingX;

			// アイコン画像(選択中はHubMenuInputSystemがIntensityを上げて光らせる)
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

			// ラベル(アイコンの下、水平中央揃え)
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

		// 操作案内。ボタン表示名は入力デバイスに応じてHubMenuInputSystemが毎フレーム更新するため、
		// ここでは空文字のままでよい
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Y = labelY + kLabelTextSize + 60.0f;
			text.Size = 26.0f;
			text.Color = { 0.6f, 0.6f, 0.6f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::HubGuideUiTag>(entity);
		}
	}

	REGISTER_SCENE_AS(HubScene, HUB_SCENE_NAME);
}
