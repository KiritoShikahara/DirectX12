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

		// Titleと違い、この背景の上にはカード等の読みやすさが必要なUIが乗るため、
		// Titleと同じ強さ(Base5.0±2.5=2.5~7.5倍)だと周期的に背景が白飛びし、
		// 半透明パネル越しにその白さが透けて配色(Orbの黄色等)が薄く見えてしまう。
		// 控えめな明滅に留める。
		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 0.3f;
		glow.BaseIntensity = 1.2f;
		glow.Frequency = 0.7f;
		glow.PhaseOffset = 0.0f;

		// 音楽
		PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7);
	}

	void MenuScene::CreateSpells()
	{
		// 必要パラメ
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& texManager = ::graphics::TextureManager::Get();

		// データ(ID=1001/1002/1003の並びで武器アイコン画像のパスを持つ。
		// Assets/Bin/CSV/MenuSpells.csv、GameSceneFactory::CreatePlayerのID対応表と一致させること)
		const auto& datas = ::data::DataRegistry::Get().GetManager<::data::SpellMenuData>().GetAll();

		// 状態管理コンポーネント
		auto ent_MenuController = manager.CreateEntity();
		auto& MenuControllerComp = manager.AddComponent<::ecs::MenuControllerComp>(ent_MenuController);
		MenuControllerComp.WindowWidth = static_cast<float>(::sys::Window::Get().GetVirtualWidth());

		// MenuPagingSystem の TargetX 計算式(centerX + diff * WindowWidth、diff=pageIndex-CurrentlySelectedIdx)
		// と初期配置を一致させるための中央オフセット。これが無いと選択中(page0)がX=0(画面左端)を
		// 中心に配置され、Pivot={0.5,0.5}のため画像の左半分が画面外に出た状態で表示されてしまう。
		const float centerX = MenuControllerComp.WindowWidth * 0.5f;
		const float centerY = static_cast<float>(::sys::Window::Get().GetVirtualHeight()) * 0.5f;

		// レイアウト定数。1ページ = 武器イメージカラーの背景パネル(奥、固定サイズの発光板) +
		// カード画像(手前、中央)の2要素で構成する。
		// 新しいWeaponSelect素材(Fire/Thunder/Orb.png)は武器名・アイコン・説明文が
		// 1枚の画像に既に合成されているため、以前のような名前/説明テキストの個別生成は行わない。
		// 素材ごとに縦横比が異なる(Fire=1213x1734、Thunder=1333x1691、Orb=1865x1734)ため、
		// 高さ基準で統一して収める(kCardMaxWidthは最も横長なOrbが幅で頭打ちにならない値に
		// 余裕を持たせてあり、実質的に全カードが同じ高さ(kCardMaxHeight)に揃う)。
		// 元画像は素材そのままだと画面に対して大きすぎるため、旧アイコン+テキスト構成時の
		// 表示占有面積(アイコン380+ラベル/説明文分=約550px)に近いサイズへ縮小している。
		constexpr float kCardMaxWidth = 640.0f;
		constexpr float kCardMaxHeight = 560.0f;
		// 背景パネル(発光板)はカードの一回り外側にパディングを足した固定サイズ
		// (カードサイズと独立した値にすると調整時に食い違うため、パディングだけを定数化する)。
		constexpr float kAccentPanelPadding = 50.0f;
		constexpr float kAccentPanelWidth = kCardMaxWidth + kAccentPanelPadding * 2.0f;
		constexpr float kAccentPanelHeight = kCardMaxHeight + kAccentPanelPadding * 2.0f;
		// kNeonIntensity(旧2.2)はSprite描画がColor*Intensityを素通しするため、
		// 特にOrbの黄(R,G成分が高い)でR/G成分が1.0を超えて白飛びし、
		// 半透明合成後も白っぽく見える不具合の原因だった。1.0にして色を素直に出す。
		constexpr float kAccentAlpha = 0.85f;        // 背景パネルの不透明度(色をはっきり見せるため引き上げ)
		constexpr float kNeonIntensity = 1.0f;

		// 背景(タイトル画像流用)の上にカードが乗ると読みづらいため、黒半透明の板を
		// カード全体の奥に敷く(UiPanelUtility参照)。ページ送りでカードはスライドするが、
		// この板自体の画面位置(centerX/centerY)は常に同じなため、スライドさせず画面固定でよい。
		{
			constexpr float kPanelPadX = 40.0f;
			constexpr float kPanelPadY = 40.0f;

			::ecs::uiutil::CreateTranslucentPanel(
				centerX, centerY,
				kAccentPanelWidth + kPanelPadX * 2.0f, kAccentPanelHeight + kPanelPadY * 2.0f,
				-2); // 色付きタイント(offset-1)・武器アクセントパネル(offset0)より奥
		}

		// 読み込み成功したページ数
		uint32_t pageIndex = 0;

		// MenuSlideComp/SpellMenuDataCompの付与だけを共通化するヘルパー。実座標(Transform)は
		// 要素の種類ごとに異なるため呼び出し側で設定する。
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

			// 武器イメージカラーの背景パネル(カードの奥、固定サイズの発光板。
			// Fire=赤/Thunder=青/Orb=黄)。
			// Shape(円)は使わない: このエンジンはSprite→Shape→Textの順で完全に別の描画パスに
			// 分かれており、Shapeは常に全Spriteより後(手前)に描画されるため、Shapeで作った円の
			// 上にカード画像(Sprite)を重ねることができない(Layer値をいくら調整しても解決しない、
			// パス自体が別なので無関係)。同じSpriteパス内で完結させ、Layer順(パネルoffset0 <
			// カードoffset1)通りにカードが手前に来るようにする。
			{
				::graphics::Color panelColor = ::ecs::menuvisuals::GetWeaponAccentColor(data.ID);
				panelColor.a = kAccentAlpha;

				auto entity = manager.CreateEntity();
				auto& transform = manager.AddComponent<::ecs::Transform>(entity);
				transform.Set2DPosition(restX, centerY);

				auto panelTexture = texManager.GetOrLoad("Assets/Effect/Texture/White.png");
				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, panelTexture);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { kAccentPanelWidth, kAccentPanelHeight };
				sprite.Color = panelColor;
				sprite.Intensity = kNeonIntensity;
				sprite.SetLayer(::ecs::SpriteLayer::UI, 0);

				attachSlide(entity, restX, data.ID);
			}

			// カード画像(パネルの手前、中央。武器名・アイコン・説明文を1枚に合成済み。
			// 元画像のアスペクト比を維持したままkCardMaxWidth×kCardMaxHeightの枠に収める)
			{
				const float texW = texRes->GetWidth();
				const float texH = texRes->GetHeight();
				const float fitScale = std::min(kCardMaxWidth / texW, kCardMaxHeight / texH);

				auto entity = manager.CreateEntity();
				auto& transform = manager.AddComponent<::ecs::Transform>(entity);
				transform.Set2DPosition(restX, centerY);

				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texRes);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { texW * fitScale, texH * fitScale };
				sprite.SetLayer(::ecs::SpriteLayer::UI, 1);

				attachSlide(entity, restX, data.ID);
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
			constexpr float kGuideTextSize = 26.0f;
			constexpr float kGuidePanelWidth = 620.0f;
			constexpr float kGuidePanelPadY = 16.0f;

			// guideCenterYは文字とパネルの見た目上の縦中心に置きたい座標(Title/Hubと同じ方式)。
			// TextComponent::Yはベースライン座標なので、そのままguideCenterYを渡すと
			// パネルより上に見えてしまう。MeasureVerticalCenterOffsetでベースラインYへ変換する。
			const float guideCenterY = static_cast<float>(::sys::Window::Get().GetVirtualHeight()) - 80.0f;
			auto& textRenderer = ::graphics::TextRenderer::Get();

			::ecs::uiutil::CreateTranslucentPanel(
				centerX, guideCenterY,
				kGuidePanelWidth, kGuideTextSize + kGuidePanelPadY * 2.0f,
				0);

			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.X = centerX;
			text.Y = guideCenterY + textRenderer.MeasureVerticalCenterOffset(kGuideTextSize);
			text.Size = kGuideTextSize;
			text.Color = { 0.8f, 0.8f, 0.8f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::MenuGuideUiTag>(entity);
		}
	}



	REGISTER_SCENE_AS(MenuScene, MENU_SCENE_NAME);

}
