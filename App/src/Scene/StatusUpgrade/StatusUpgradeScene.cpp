#include "apppch.h"
#include "StatusUpgradeScene.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include
#include<ecs/system/manager/ComponentSystemManager.h>

#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/StatusUpgrade/StatusUpgradeComponent.h>
#include<system/StatusUpgrade/StatusUpgradeInputSystem.h>
#include<system/StatusUpgrade/StatusUpgradeLabels.h>
#include<system/UI/UiPanelUtility.h>

#include<Data/StatUpgrade/StatUpgradeData.h>
#include<Data/Save/PlayerSaveData.h>

#include"../macros.h"

namespace
{
	/// <summary>
	/// data::eStatUpgradeType(0..7)に対応する表示アイコンのパス。
	/// 専用アイコン未作成の項目(クールダウン短縮・ゴールド獲得量)はloading.pngを代用する
	/// (PerkSelectSystem::GetPerkIconPath/WeaponIconRegistryと同じ、後で差し替え前提の方針)。
	/// </summary>
	const char* GetStatUpgradeIconPath(int index)
	{
		constexpr const char* kFallbackIcon = "Assets/Icon/loading.png";

		switch (static_cast<data::eStatUpgradeType>(index))
		{
		case data::eStatUpgradeType::MaxHp:              return "Assets/Icon/health.png";
		case data::eStatUpgradeType::AtkPower:           return "Assets/Icon/attack_up.png";
		case data::eStatUpgradeType::Defense:            return "Assets/Icon/defense.png";
		case data::eStatUpgradeType::MoveSpeed:          return "Assets/Icon/speed_up.png";
		case data::eStatUpgradeType::HpRegen:            return "Assets/Icon/heel.png";
		case data::eStatUpgradeType::ExperienceGainRate: return "Assets/Icon/exp_up.png";

		case data::eStatUpgradeType::CooldownRate: // 専用アイコン未作成
		case data::eStatUpgradeType::GoldGainRate: // 専用アイコン未作成
		default:
			return kFallbackIcon;
		}
	}
}

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

#if DEV_TOOL_ENABLED
		mPlayerSaveDebugPanel = std::make_unique<debug::PlayerSaveDebugPanel>("StatusUpgradeScene_PlayerSaveDebug");
#endif

		DEBUG_LOG(::sys::eLogLevel::Log, "StatusUpgrade Scene.");
	}

	void StatusUpgradeScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
		::audio::AudioManager::Get().ClearSceneSounds();

#if DEV_TOOL_ENABLED
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
		const float centerY = static_cast<float>(window.GetVirtualHeight()) * 0.5f;

		// カーソル状態を保持するコントローラーエンティティ
		auto controllerEntity = manager.CreateEntity();
		manager.AddComponent<::ecs::StatusUpgradeComponent>(controllerEntity);

		// 所持ゴールド（画面最上部。実際の値はStatusUpgradeInputSystemが毎フレーム更新する）
		constexpr float kGoldY = 60.0f;
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX - 150.0f;
			text.Y = kGoldY;
			text.Size = 36.0f;
			text.Color = { 1.0f, 0.85f, 0.3f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::StatusUpgradeGoldUiTag>(entity);
		}

		// 見出し
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = L"ステータス強化";
			text.X = centerX - 150.0f;
			text.Y = 140.0f;
			text.Size = 48.0f;
			text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			text.Layer = 10;
		}

		// 8ステータス分のカード(名前(上)/アイコン(中)/強化状態(下)の縦積み)を
		// 横4×縦2グリッドで配置する。テキスト内容・色は毎フレームStatusUpgradeInputSystemが更新するため、
		// ここでは空文字のままでよい(アイコンのみここで確定させ、以後変化しない)。
		constexpr int kCardColumns = 4;
		constexpr int kCardRows = 2;
		static_assert(kCardColumns * kCardRows == ::ecs::StatusUpgradeComponent::kOptionCount,
			"カード数はStatusUpgradeComponent::kOptionCountと一致させること");

		constexpr float kIconSize = 100.0f;      // アイコンサイズ
		constexpr float kCardSpacingX = 380.0f;  // カード中心どうしの横間隔(px)
		constexpr float kCardSpacingY = 240.0f;  // カード中心どうしの縦間隔(px)
		constexpr float kGridCenterY = 452.0f;   // グリッド全体の中心Y(見出しと操作案内の間)
		constexpr float kNameGapY = 34.0f;       // 名前テキストとアイコン上端の間隔(px)
		constexpr float kStateGapY = 14.0f;      // 強化状態テキストとアイコン下端の間隔(px)
		constexpr float kNameTextSize = 26.0f;
		constexpr float kStateTextSize = 24.0f;

		for (int i = 0; i < ::ecs::StatusUpgradeComponent::kOptionCount; ++i)
		{
			const int col = i % kCardColumns;
			const int row = i / kCardColumns;
			const float x = centerX + (static_cast<float>(col) - (kCardColumns - 1) * 0.5f) * kCardSpacingX;
			const float iconY = kGridCenterY + (static_cast<float>(row) - (kCardRows - 1) * 0.5f) * kCardSpacingY;

			// 名前(アイコンの上)
			{
				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
				text.X = x; // StatusUpgradeInputSystemがMeasureWidthで中央揃えに書き換える
				text.Y = iconY - kIconSize * 0.5f - kNameGapY;
				text.Size = kNameTextSize;
				text.Color = { 0.7f, 0.7f, 0.7f, 1.0f };
				text.Layer = 10;

				registry.emplace<::ecs::StatusUpgradeCardUiTag>(entity,
					::ecs::StatusUpgradeCardUiTag{ i, ::ecs::eStatusUpgradeCardElement::NameText, x });
			}

			// アイコン本体(項目ごとに固定、以後変化しないためここで確定させる)
			{
				auto entity = manager.CreateEntity();
				auto& tr = manager.AddComponent<::ecs::Transform>(entity);
				tr.Set2DPosition(x, iconY);

				auto tex = ::graphics::TextureManager::Get().GetOrLoad(GetStatUpgradeIconPath(i));
				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, tex);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { kIconSize, kIconSize };
				sprite.SetLayer(::ecs::SpriteLayer::UI, 3);

				registry.emplace<::ecs::StatusUpgradeCardUiTag>(entity,
					::ecs::StatusUpgradeCardUiTag{ i, ::ecs::eStatusUpgradeCardElement::Icon, x });
			}

			// 強化状態(Lv./コスト等。アイコンの下)
			{
				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
				text.X = x; // StatusUpgradeInputSystemがMeasureWidthで中央揃えに書き換える
				text.Y = iconY + kIconSize * 0.5f + kStateGapY;
				text.Size = kStateTextSize;
				text.Color = { 0.7f, 0.7f, 0.7f, 1.0f };
				text.Layer = 10;

				registry.emplace<::ecs::StatusUpgradeCardUiTag>(entity,
					::ecs::StatusUpgradeCardUiTag{ i, ::ecs::eStatusUpgradeCardElement::StateText, x });
			}
		}

		// 操作案内(2行)。ボタン表示名は入力デバイス(キーボード/マウス or パッド)によって
		// 変わる(「Selectって何ボタン？」を防ぐため)ので、内容はStatusUpgradeInputSystemが
		// 毎フレーム更新する。ここでは空文字のままでよい
		constexpr float guideY = kGridCenterY + (kCardRows - 1) * 0.5f * kCardSpacingY
			+ kIconSize * 0.5f + kStateGapY + kStateTextSize + 40.0f;
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX - 260.0f;
			text.Y = guideY;
			text.Size = 26.0f;
			text.Color = { 0.6f, 0.6f, 0.6f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::StatusUpgradeGuideUiTag>(entity, ::ecs::StatusUpgradeGuideUiTag{ 0 });
		}

		// 操作案内の2行目(一括強化・リセットは行が長くなるため分ける)
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX - 260.0f;
			text.Y = guideY + 32.0f;
			text.Size = 26.0f;
			text.Color = { 0.6f, 0.6f, 0.6f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::StatusUpgradeGuideUiTag>(entity, ::ecs::StatusUpgradeGuideUiTag{ 1 });
		}

		// 「強化しますか？」の確認ダイアログ。背景に負けて読みづらいとの指摘を受け、
		// 画面中心に黒背景を敷いた上に、その背景の中心へテキストを重ねる構成にする。
		// 半透明だと背景が透けて文字が読みづらくなるため完全不透明(Alpha=1.0)にし、
		// テキストが箱からはみ出さないよう十分な余白を持たせたサイズにする。
		// (IsConfirming中のみ表示。位置・表示有無はStatusUpgradeInputSystem::RefreshTextsが毎フレーム更新する)
		constexpr float kConfirmWindowWidth = 1100.0f;
		constexpr float kConfirmWindowHeight = 340.0f;
		{
			auto entity = ::ecs::uiutil::CreateTranslucentPanel(
				centerX, centerY,
				kConfirmWindowWidth, kConfirmWindowHeight,
				10,    // 読みやすさ用ウィンドウ(offset0)・アイコン(offset3)より手前
				1.0f); // 完全不透明の黒(半透明だと背景が透けて文字が読みづらくなるため)
			registry.get<::ecs::Sprite>(entity).IsVisible = false;

			registry.emplace<::ecs::StatusUpgradeConfirmWindowUiTag>(entity);
		}
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX; // StatusUpgradeInputSystemがMeasureWidthで中央揃えに書き換える
			text.Y = centerY; // StatusUpgradeInputSystemが行数から算出した値に書き換える
			text.Size = 28.0f;
			text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			text.Layer = 20;

			registry.emplace<::ecs::StatusUpgradeConfirmUiTag>(entity);
		}

		// フィードバックメッセージ（「ゴールドが足りません」等。MessageTimerが尽きたら非表示）
		constexpr float kMessageOffsetY = 130.0f;
		constexpr float kMessageTextSize = 28.0f;
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX - 150.0f;
			text.Y = guideY + kMessageOffsetY;
			text.Size = kMessageTextSize;
			text.Color = { 1.0f, 0.4f, 0.4f, 1.0f };
			text.Layer = 20;

			registry.emplace<::ecs::StatusUpgradeMessageUiTag>(entity);
		}

		// 背景(タイトル画像流用、不透明)の上に文字が乗ると読みづらいため、
		// ゴールド〜メッセージ全体を覆う黒半透明ウィンドウを敷く(UiPanelUtility参照)。
		// 描画順は「Layerが大きいほど前面」(SpriteRenderer参照)なので、背景(Background)より
		// 手前・アイコン(offset3)より奥になるようoffset0にする。
		{
			constexpr float kWindowPadTop = 30.0f;
			constexpr float kWindowPadBottom = 30.0f;
			// 名前テキストが長い項目(「クールダウン短縮」「ゴールド獲得量」等)が
			// ウィンドウ内に収まるための左右余白
			constexpr float kWindowPadX = 250.0f;

			const float windowTop = kGoldY - kWindowPadTop;
			const float windowBottom = guideY + kMessageOffsetY + kMessageTextSize + kWindowPadBottom;
			const float windowWidth = kCardSpacingX * static_cast<float>(kCardColumns - 1) + kIconSize + kWindowPadX * 2.0f;

			::ecs::uiutil::CreateTranslucentPanel(
				centerX, (windowTop + windowBottom) * 0.5f,
				windowWidth, windowBottom - windowTop,
				0); // アイコン(offset3)より奥
		}
	}

	REGISTER_SCENE_AS(StatusUpgradeScene, STATUS_UPGRADE_SCENE_NAME);
}
