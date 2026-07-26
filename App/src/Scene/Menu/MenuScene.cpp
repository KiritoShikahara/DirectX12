#include "apppch.h"
#include "MenuScene.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include
#include"../macros.h"

#include<Data/Menu/MenuSpellsData.h>
#include<system/MenuController/MenuControllerComp.h>
#include<system/MenuController/WeaponSelectVisuals.h>
#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/MenuController/MenuControllerSystem.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/UI/UiPanelUtility.h>
#include<graphics/Text/Renderer/TextRenderer.h> // ラベルをアイコン中央へ揃えるため、幅を実測する

#include<algorithm>

namespace scene
{
	void MenuScene::Initialize()
	{
		// TimeScaleはプロセス全体で共有され、シーンを跨いでも持ち越される。
		// GameOver等でTimeScale=0.0のままTitle→Menuへ遷移してくるケースがあるため、
		// 一時停止の概念が無いMenuでは必ず1.0へ戻す
		// （MenuSlideSystemはdeltaTime依存のため、0.0のままだと選択インデックスは
		// 正しく更新されるのにスライド移動だけ起きない、という不具合になる）。
		GetTime().SetTimeScale(1.0);

		// システム
		CreateUserSystem();

		// データ
		LoadData();

		// 背景
		CreateBG();

		// スペル
		CreateSpells();

		DEBUG_LOG(::sys::eLogLevel::Log, "Menu Scene.");
	}

	void MenuScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
		::audio::AudioManager::Get().ClearSceneSounds();

#if DEV_TOOL_ENABLED
		// シーンを抜けるタイミングで、このシーンが登録したデバッグUIを解除する
		::sys::ImGuiManager::Get().RemoveDebugUI("MenuScene_SpellMenuData");
#endif
	}


	void MenuScene::LoadData()
	{

		auto& dataReg = ::data::DataRegistry::Get();
		if (dataReg.IsRegistered<::data::SpellMenuData>() == false)
		{
			dataReg.Register<::data::SpellMenuData>("Assets/Bin/CSV/MenuSpells.csv");
		}

		// データすべて読み込み
		dataReg.LoadAll();

#if DEV_TOOL_ENABLED
		// static で1回だけ構築（DataManager<T>& の参照を持つだけの軽量クラス）
		static ::data::DataInspector<::data::SpellMenuData> sSpellInspector{
			dataReg.GetManager<::data::SpellMenuData>()
		};

		// key を指定して登録。同じ key で再登録すると上書きされるので
		// シーン再入場時に多重登録される心配はない。
		::sys::ImGuiManager::Get().AddDebugUI([]()
			{
				sSpellInspector.Draw("Spell Menu Data");
			}, "MenuScene_SpellMenuData");
#endif
	}

	void MenuScene::CreateUserSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();

		manager.AddUserSystem<::ecs::MenuInputSystem>(::ecs::eUpdatePhase::PreUpdate);
		manager.AddUserSystem<::ecs::MenuPagingSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::MenuSlideSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::MenuSelectInputSystem>(::ecs::eUpdatePhase::PostUpdate);
	}

	void MenuScene::CreateBG()
	{
		// 背景のインスタンス生成(仮素材だったMenu専用背景から、Title/Hub/StatusUpgradeと
		// 同じ背景素材に統一する)
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920,1080 };
		sprite.Intensity = 1.0f;
		sprite.SetLayer(::ecs::SpriteLayer::Background);

		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 2.5f;
		glow.BaseIntensity = 5.0f;
		glow.Frequency = 0.7f;
		glow.PhaseOffset = 0.0f;

		// 画面全体の色付きオーバーレイ(選択中の武器のイメージカラーで軽く染める。
		// 実際の色更新はMenuPagingSystemが毎フレーム行う。WeaponSelectVisuals.h参照)
		{
			auto tintEntity = manager.CreateEntity();
			auto& tintTransform = manager.AddComponent<::ecs::Transform>(tintEntity);
			tintTransform.Set2DPosition(0.0f, 0.0f);

			auto whiteTexture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Effect/Texture/White.png");
			auto& tintSprite = manager.AddComponent<::ecs::Sprite>(tintEntity, whiteTexture);
			tintSprite.Size = { 1920.0f, 1080.0f };
			tintSprite.SetLayer(::ecs::SpriteLayer::UI, -1); // 背景(Background)より手前、円(UI+0)より奥

			registry.emplace<::ecs::MenuBackgroundTintUiTag>(tintEntity);
		}

		// 音楽
		PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7);
	}

	void MenuScene::CreateSpells()
	{
		// 必要パラメ
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& texManager = ::graphics::TextureManager::Get();
		auto& textRenderer = ::graphics::TextRenderer::Get();

		// データ(ID=1001/1002/1003の並びで武器アイコン画像のパスを持つ。
		// Assets/Bin/CSV/MenuSpells.csv、GameSceneFactory::CreatePlayerのID対応表と一致させること)
		const auto& datas = ::data::DataRegistry::Get().GetManager<::data::SpellMenuData>().GetAll();

		// 表示ラベル・説明文。データの並び順(ID=1001,1002,1003)と対応させる
		// (UTF-8→UTF-16変換ユーティリティを新設せずに済むよう、wstringリテラルで直接持つ。
		// StatusUpgradeScene等の他画面と同じ方針)。
		const std::wstring kLabels[] = { L"Fire", L"Lightning", L"Orb" };
		// フォントアトラス(font.json)に含まれない文字("、""基""敵"等)は無音で欠落するため、
		// 収録済みの文字だけで構成すること(収録有無の確認はmsdf-atlas-genの出力jsonを直接参照)
		const std::wstring kDescriptions[] =
		{
			L"左クリックで火の弾を放つ武器",
			L"右クリックで周囲に範囲攻撃を放つ",
			L"常時プレイヤーを周回して自動でダメージを与える",
		};

		// 状態管理コンポーネント
		auto ent_MenuController = manager.CreateEntity();
		auto& MenuControllerComp = manager.AddComponent<::ecs::MenuControllerComp>(ent_MenuController);
		MenuControllerComp.WindowWidth = static_cast<float>(::sys::Window::Get().GetVirtualWidth());

		// MenuPagingSystem の TargetX 計算式(centerX + diff * WindowWidth、diff=pageIndex-CurrentlySelectedIdx)
		// と初期配置を一致させるための中央オフセット。これが無いと選択中(page0)がX=0(画面左端)を
		// 中心に配置され、Pivot={0.5,0.5}のため画像の左半分が画面外に出た状態で表示されてしまう。
		const float centerX = MenuControllerComp.WindowWidth * 0.5f;
		const float centerY = static_cast<float>(::sys::Window::Get().GetVirtualHeight()) * 0.5f;

		// レイアウト定数。1ページ = 武器名(上) + 武器イメージカラーの背景パネル(奥)+武器アイコン(手前、中央) +
		// 説明文(下)の4要素で構成する(以前は1枚の全画面画像を表示するだけだったが、
		// Weapon_Selectのアイコン素材を使って武器選択画面らしい見た目にするため)。
		constexpr float kCircleSize = 700.0f;   // 背景パネルの一辺
		constexpr float kIconMaxSize = 380.0f;  // 武器アイコンの最大表示サイズ(縦横とも。アスペクト比は維持)
		constexpr float kLabelGapY = 44.0f;     // アイコン上端とラベル(武器名)の間隔
		constexpr float kLabelTextSize = 42.0f;
		constexpr float kDescGapY = 44.0f;      // アイコン下端と説明文の間隔
		constexpr float kDescTextSize = 26.0f;
		constexpr float kCircleAlpha = 0.55f;   // 背景パネルの不透明度(武器イメージカラーに適用)
		constexpr float kNeonIntensity = 2.2f;  // 発光感を出すための輝度倍率

		// アイコンの実表示サイズ(元画像のアスペクト比依存)ではなく、レイアウト用の固定枠
		// (kIconMaxSize角)を基準にラベル/説明文の位置を揃える(武器ごとに画像の縦横比が
		// 異なる=Fire.pngは3:2、lighting.png/orb.pngは1:1、ため実サイズ基準だと行がズレる)。
		const float nameY = centerY - kIconMaxSize * 0.5f - kLabelGapY - kLabelTextSize;
		const float descY = centerY + kIconMaxSize * 0.5f + kDescGapY;

		// 背景(タイトル画像流用)の上に文字/アイコンが乗ると読みづらいため、黒半透明の板を
		// カード全体の奥に敷く(UiPanelUtility参照)。ページ送りで武器アイコン/名前/説明文は
		// スライドするが、カード自体の画面位置(centerX/centerY)は常に同じなため、
		// この板はスライドさせず画面固定のままでよい。
		{
			constexpr float kPanelPadX = 60.0f;
			constexpr float kPanelPadTop = 30.0f;
			constexpr float kPanelPadBottom = 30.0f;

			const float panelTop = nameY - kPanelPadTop;
			const float panelBottom = descY + kDescTextSize + kPanelPadBottom;
			const float panelWidth = kCircleSize + kPanelPadX * 2.0f;

			::ecs::uiutil::CreateTranslucentPanel(
				centerX, (panelTop + panelBottom) * 0.5f,
				panelWidth, panelBottom - panelTop,
				-2); // 色付きタイント(offset-1)・武器アクセントパネル(offset0)より奥
		}

		// 読み込み成功したページ数
		uint32_t pageIndex = 0;

		// MenuSlideComp/SpellMenuDataCompの付与だけを共通化するヘルパー。実座標(Transform/TextComponent)は
		// 要素の種類ごとに異なるため呼び出し側で設定する
		// (MenuSlideSystemはTransform持ちとTextComponent持ちの両方に対応済み。下記参照)。
		auto attachSlide = [&](entt::entity entity, float targetX, uint32_t spellId)
			{
				auto& slide = manager.AddComponent<::ecs::MenuSlideComp>(entity);
				slide.TargetX = targetX;

				auto& spellComp = manager.AddComponent<::ecs::SpellMenuDataComp>(entity);
				spellComp.SpellID = spellId;
				spellComp.PageIndex = pageIndex;
			};

		// エンティティ達
		for (auto& data : datas)
		{
			if (data.TexPath.empty()) continue;

			// リソース
			auto texRes = texManager.GetOrLoad(data.TexPath);
			if (!texRes) continue;

			// 初期座標(MenuPagingSystemのTargetX計算式と一致させる)
			const float restX = centerX + static_cast<float>(pageIndex) * MenuControllerComp.WindowWidth;

			// 武器イメージカラーの背景パネル(アイコンの奥。Fire=赤/Lightning=青/Orb=黄)。
			// Shape(円)は使わない: このエンジンはSprite→Shape→Textの順で完全に別の描画パスに
			// 分かれており、Shapeは常に全Spriteより後(手前)に描画されるため、Shapeで作った円の
			// 上に武器アイコン(Sprite)を重ねることができない(Layer値をいくら調整しても解決しない、
			// パス自体が別なので無関係)。同じSpriteパス内で完結させ、Layer順(円offset0 <
			// アイコンoffset1)通りにアイコンが手前に来るようにする。
			{
				::graphics::Color panelColor = ::ecs::menuvisuals::GetWeaponAccentColor(data.ID);
				panelColor.a = kCircleAlpha;

				auto entity = manager.CreateEntity();
				auto& transform = manager.AddComponent<::ecs::Transform>(entity);
				transform.Set2DPosition(restX, centerY);

				auto panelTexture = texManager.GetOrLoad("Assets/Effect/Texture/White.png");
				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, panelTexture);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { kCircleSize, kCircleSize };
				sprite.Color = panelColor;
				sprite.Intensity = kNeonIntensity;
				sprite.SetLayer(::ecs::SpriteLayer::UI, 0);

				attachSlide(entity, restX, data.ID);
			}

			// 武器アイコン(円の手前、中央。元画像のアスペクト比を維持したまま最大kIconMaxSize角に収める)
			{
				const float texW = texRes->GetWidth();
				const float texH = texRes->GetHeight();
				const float fitScale = std::min(kIconMaxSize / texW, kIconMaxSize / texH);

				auto entity = manager.CreateEntity();
				auto& transform = manager.AddComponent<::ecs::Transform>(entity);
				transform.Set2DPosition(restX, centerY);

				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texRes);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { texW * fitScale, texH * fitScale };
				sprite.SetLayer(::ecs::SpriteLayer::UI, 1);

				attachSlide(entity, restX, data.ID);
			}

			// 武器名(アイコンの上、水平中央揃え)
			{
				const std::wstring label = (pageIndex < sizeof(kLabels) / sizeof(kLabels[0])) ? kLabels[pageIndex] : L"";
				const float textWidth = textRenderer.MeasureWidth(label, kLabelTextSize);
				const float textX = restX - textWidth * 0.5f;

				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
				text.Text = label;
				text.X = textX;
				text.Y = nameY;
				text.Size = kLabelTextSize;
				text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
				text.Layer = 10;

				attachSlide(entity, textX, data.ID);
			}

			// 説明文(アイコンの下、水平中央揃え)
			{
				const std::wstring desc = (pageIndex < sizeof(kDescriptions) / sizeof(kDescriptions[0])) ? kDescriptions[pageIndex] : L"";
				const float textWidth = textRenderer.MeasureWidth(desc, kDescTextSize);
				const float textX = restX - textWidth * 0.5f;

				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
				text.Text = desc;
				text.X = textX;
				text.Y = descY;
				text.Size = kDescTextSize;
				text.Color = { 0.85f, 0.85f, 0.85f, 1.0f };
				text.Layer = 10;

				attachSlide(entity, textX, data.ID);
			}

			if (pageIndex == 0)
			{
				MenuControllerComp.ActiveSpellID = data.ID;
			}

			++pageIndex;
		}

		MenuControllerComp.TotalPages = pageIndex;

		// 操作案内(画面固定、ページ送りでスライドさせない)。ボタン表示名は入力デバイスに応じて
		// MenuInputSystemが毎フレーム更新するため、ここでは空文字のままでよい
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX;
			text.Y = static_cast<float>(::sys::Window::Get().GetVirtualHeight()) - 80.0f;
			text.Size = 26.0f;
			text.Color = { 0.8f, 0.8f, 0.8f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::MenuGuideUiTag>(entity);
		}
	}



	REGISTER_SCENE_AS(MenuScene, MENU_SCENE_NAME);

}
