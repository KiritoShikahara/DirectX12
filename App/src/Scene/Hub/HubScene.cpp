#include "apppch.h"
#include "HubScene.h"
#include<ecs/system/manager/ComponentSystemManager.h>

#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/HubMenu/HubMenuComponent.h>
#include<system/HubMenu/HubMenuInputSystem.h>

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

		auto controllerEntity = manager.CreateEntity();
		manager.AddComponent<::ecs::HubMenuComponent>(controllerEntity);

		// レイアウト定数（仮想解像度1280x720基準、画面中央に2択を横並び。個人開発プロトタイプの暫定値）
		constexpr float kOptionY = 500.0f;
		constexpr float kOptionSpacingX = 360.0f;
		constexpr float kOptionTextSize = 40.0f;
		const float centerX = static_cast<float>(window.GetVirtualWidth()) * 0.5f;

		const std::wstring labels[::ecs::HubMenuComponent::kChoiceCount] =
		{
			L"武器・ステージ選択",
			L"ステータス強化",
		};

		for (int i = 0; i < ::ecs::HubMenuComponent::kChoiceCount; ++i)
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = labels[i];
			text.X = centerX + static_cast<float>(i) * kOptionSpacingX - kOptionSpacingX * 0.5f - 150.0f;
			text.Y = kOptionY;
			text.Size = kOptionTextSize;
			text.Color = (i == 0) ? DirectX::XMFLOAT4{ 1.0f, 0.9f, 0.2f, 1.0f } : DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::HubMenuOptionUiTag>(entity, i);
		}
	}

	REGISTER_SCENE_AS(HubScene, HUB_SCENE_NAME);
}
