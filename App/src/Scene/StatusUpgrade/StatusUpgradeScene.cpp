#include "apppch.h"
#include "StatusUpgradeScene.h"
#include<ecs/system/manager/ComponentSystemManager.h>

#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/StatusUpgrade/StatusUpgradeComponent.h>
#include<system/StatusUpgrade/StatusUpgradeInputSystem.h>

#include<Data/StatUpgrade/StatUpgradeData.h>
#include<Data/Save/PlayerSaveData.h>

#include"../macros.h"

namespace scene
{
	void StatusUpgradeScene::Initialize()
	{
		// TimeScaleはプロセス全体で共有され、シーンを跨いでも持ち越される。
		// 一時停止の概念が無いこのシーンでは必ず1.0へ戻す（HubScene/MenuSceneと同じ理由）。
		GetTime().SetTimeScale(1.0);

		LoadData();

		CreateCompSystem();

		LoadResource();
		CreateBackground();
		CreateOptions();

#ifdef _DEBUG
		mPlayerSaveDebugPanel = std::make_unique<debug::PlayerSaveDebugPanel>("StatusUpgradeScene_PlayerSaveDebug");
#endif

		DEBUG_LOG(::sys::eLogLevel::Log, "StatusUpgrade Scene.");
	}

	void StatusUpgradeScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
		::audio::AudioManager::Get().ClearSceneSounds();

#ifdef _DEBUG
		mPlayerSaveDebugPanel.reset();
#endif
	}

	void StatusUpgradeScene::LoadData()
	{
		// DataRegistryはプロセス全体で1つのシングルトンのため、二重登録を避けるガードを入れる
		// （GameScene.cppと同じパターン）。
		auto& dataRegistry = data::DataRegistry::Get();
		if (!dataRegistry.IsRegistered<data::StatUpgradeData>())
		{
			dataRegistry.Register<data::StatUpgradeData>("Assets/Data/StatUpgrade/StatUpgradeData.csv");
		}
		dataRegistry.LoadAll();

		// プレイヤーの永続的な進行状況(ゴールド・強化レベル)。CSV/DBとは別系統(JSON永続化)。
		data::EnsurePlayerSaveDataLoaded();
	}

	void StatusUpgradeScene::CreateCompSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();

		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::StatusUpgradeInputSystem>(::ecs::eUpdatePhase::PostUpdate);
	}

	void StatusUpgradeScene::LoadResource()
	{
		auto& manager = graphics::TextureManager::Get();
		manager.GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");
	}

	void StatusUpgradeScene::CreateBackground()
	{
		// 専用の背景素材が無いため、タイトルと同じ背景を暫定的に流用する
		// （専用素材が用意でき次第、Assets/Texture/StatusUpgrade/配下へ差し替える）。
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

	void StatusUpgradeScene::CreateOptions()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& window = ::sys::Window::Get();

		const float centerX = static_cast<float>(window.GetVirtualWidth()) * 0.5f;

		// カーソル状態を保持するコントローラーエンティティ
		auto controllerEntity = manager.CreateEntity();
		manager.AddComponent<::ecs::StatusUpgradeComponent>(controllerEntity);

		// 見出し
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = L"ステータス強化";
			text.X = centerX - 150.0f;
			text.Y = 200.0f;
			text.Size = 48.0f;
			text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			text.Layer = 10;
		}

		// 所持ゴールド（実際の値はStatusUpgradeInputSystemが毎フレーム更新する）
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX - 150.0f;
			text.Y = 280.0f;
			text.Size = 32.0f;
			text.Color = { 1.0f, 0.85f, 0.3f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::StatusUpgradeGoldUiTag>(entity);
		}

		// 4ステータス分の選択肢（初期テキストはStatusUpgradeInputSystemの初回Updateで
		// 最新の値に上書きされるため、ここでは空文字のままでよい）
		constexpr float kOptionStartY = 380.0f;
		constexpr float kOptionSpacingY = 70.0f;
		constexpr float kOptionTextSize = 32.0f;

		for (int i = 0; i < ::ecs::StatusUpgradeComponent::kOptionCount; ++i)
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX - 250.0f;
			text.Y = kOptionStartY + static_cast<float>(i) * kOptionSpacingY;
			text.Size = kOptionTextSize;
			text.Color = { 0.7f, 0.7f, 0.7f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::StatusUpgradeOptionUiTag>(entity, i);
		}

		// 操作案内
		const float guideY = kOptionStartY + static_cast<float>(::ecs::StatusUpgradeComponent::kOptionCount) * kOptionSpacingY + 60.0f;
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = L"↑/↓:選択　Select:強化　→:最大まで強化";
			text.X = centerX - 260.0f;
			text.Y = guideY;
			text.Size = 26.0f;
			text.Color = { 0.6f, 0.6f, 0.6f, 1.0f };
			text.Layer = 10;
		}

		// 操作案内の2行目(一括強化・リセットは行が長くなるため分ける)
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = L"Delete:全リセット(全額払い戻し)　Cancel:戻る";
			text.X = centerX - 260.0f;
			text.Y = guideY + 32.0f;
			text.Size = 26.0f;
			text.Color = { 0.6f, 0.6f, 0.6f, 1.0f };
			text.Layer = 10;
		}

		// 「強化しますか？」の確認ダイアログ（IsConfirming中のみ表示。空文字時は非表示のまま）
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX - 260.0f;
			text.Y = guideY + 60.0f;
			text.Size = 30.0f;
			text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			text.Layer = 20;

			registry.emplace<::ecs::StatusUpgradeConfirmUiTag>(entity);
		}

		// フィードバックメッセージ（「ゴールドが足りません」等。MessageTimerが尽きたら非表示）
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX - 150.0f;
			text.Y = guideY + 130.0f;
			text.Size = 28.0f;
			text.Color = { 1.0f, 0.4f, 0.4f, 1.0f };
			text.Layer = 20;

			registry.emplace<::ecs::StatusUpgradeMessageUiTag>(entity);
		}
	}

	REGISTER_SCENE_AS(StatusUpgradeScene, STATUS_UPGRADE_SCENE_NAME);
}
