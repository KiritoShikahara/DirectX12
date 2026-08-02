#include "apppch.h"
#include "MenuScene.h"
#include <Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include
#include "../macros.h"
#include <Data/Menu/MenuSpellsData.h>
#include <system/MenuController/MenuControllerComp.h>
#include <system/MenuController/WeaponSelectVisuals.h>
#include <system/GlowAnimation/GlowAnimationComp.h>
#include <system/MenuController/MenuControllerSystem.h>
#include <system/GlowAnimation/SpriteGlowSystem.h>
#include <system/UI/UiPanelUtility.h>
#include <graphics/Text/Renderer/TextRenderer.h> // ラベルをアイコン中央へ揃えるため、幅を実測する
#include <algorithm>

namespace scene
{
	void MenuScene::Initialize()
	{
		// TimeScaleはプロセス全体で共有され、シーンを跨いでも持ち越される
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

		// key を指定して登録
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
		// 背景のインスタンス生成
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920,1080 };
		sprite.Intensity = 1.0f;
		sprite.SetLayer(::ecs::SpriteLayer::Background);

		// 控えめな明滅に留める
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

		// データ取得
		const auto& datas = ::data::DataRegistry::Get().GetManager<::data::SpellMenuData>().GetAll();

		// 状態管理コンポーネント
		auto ent_MenuController = manager.CreateEntity();
		auto& MenuControllerComp = manager.AddComponent<::ecs::MenuControllerComp>(ent_MenuController);
		MenuControllerComp.WindowWidth = static_cast<float>(::sys::Window::Get().GetVirtualWidth());

		const float centerX = MenuControllerComp.WindowWidth * 0.5f;
		const float centerY = static_cast<float>(::sys::Window::Get().GetVirtualHeight()) * 0.5f;

		// レイアウト定数
		constexpr float kCardMaxWidth = 640.0f;
		constexpr float kCardMaxHeight = 560.0f;
		constexpr float kAccentPanelPadding = 50.0f;
		constexpr float kAccentPanelWidth = kCardMaxWidth + kAccentPanelPadding * 2.0f;
		constexpr float kAccentPanelHeight = kCardMaxHeight + kAccentPanelPadding * 2.0f;
		constexpr float kAccentAlpha = 0.85f;
		constexpr float kNeonIntensity = 1.0f;

		// 背景パネル
		{
			constexpr float kPanelPadX = 40.0f;
			constexpr float kPanelPadY = 40.0f;

			::ecs::uiutil::CreateTranslucentPanel(
				centerX, centerY,
				kAccentPanelWidth + kPanelPadX * 2.0f, kAccentPanelHeight + kPanelPadY * 2.0f,
				-2);
		}

		// 読み込み成功したページ数
		uint32_t pageIndex = 0;

		// MenuSlideComp付与ヘルパー
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

			// 初期座標
			const float restX = centerX + static_cast<float>(pageIndex) * MenuControllerComp.WindowWidth;

			// 武器イメージカラーの背景パネル
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

			// カード画像
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

		// 操作案内
		{
			constexpr float kGuideTextSize = 26.0f;
			constexpr float kGuidePanelWidth = 620.0f;
			constexpr float kGuidePanelPadY = 16.0f;

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