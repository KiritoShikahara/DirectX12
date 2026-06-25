#include "apppch.h"
#include "MenuScene.h"
#include"../macros.h"

#include<Data/Menu/MenuSpellsData.h>
#include<system/MenuController/MenuControllerComp.h>
#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/MenuController/MenuControllerSystem.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>

namespace scene
{
	void MenuScene::Initialize()
	{
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

#ifdef _DEBUG
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

#ifdef _DEBUG
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
		// 背景のインスタンス生成
		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Menu/TX_MenuBG.jpg");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920,1080 };
		sprite.Intensity = 1.0f;
		sprite.SetLayer(::ecs::SpriteLayer::Background);


		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 0.5;
		glow.BaseIntensity = 1.2;
		glow.Frequency = 1.5;
		glow.PhaseOffset = 0;

		// 音楽
		PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7);
	}

	void MenuScene::CreateSpells()
	{
		// 必要パラメ
		auto& manager = ::ecs::EntityManager::Get();
		auto& texManager = ::graphics::TextureManager::Get();

		// データ
		const auto& datas = ::data::DataRegistry::Get().GetManager<::data::SpellMenuData>().GetAll();

		// 状態管理コンポーネント
		auto ent_MenuController = manager.CreateEntity();
		auto& MenuControllerComp = manager.AddComponent<::ecs::MenuControllerComp>(ent_MenuController);
		MenuControllerComp.WindowWidth = static_cast<float>(::sys::Window::Get().GetVirtualWidth());

		// 読み込み成功したページ数
		uint32_t pageIndex = 0;
		float Height = ::sys::Window::Get().GetVirtualHeight() / 2;
		float scale = 0.7f;

		// エンティティ達
		for (auto& data : datas)
		{
			if (data.TexPath.empty()) continue;

			// リソース
			auto texRes = texManager.GetOrLoad(data.TexPath);
			if (!texRes) continue;

			auto entity = manager.CreateEntity();
			auto& transform = manager.AddComponent<::ecs::Transform>(entity);
			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texRes);
			sprite.Pivot = { 0.5,0.5 };
			sprite.DrawScale = { scale,scale };

			auto& SpellID = manager.AddComponent<::ecs::SpellMenuDataComp>(entity);
			SpellID.SpellID = data.ID;
			SpellID.PageIndex = pageIndex;

			auto& slide = manager.AddComponent<::ecs::MenuSlideComp>(entity);

			// 初期座標
			const float restX = static_cast<float>(pageIndex) * MenuControllerComp.WindowWidth;
			transform.Set2DPosition(restX, Height);
			slide.TargetX = restX;

			if (pageIndex == 0)
			{
				MenuControllerComp.ActiveSpellID = data.ID;
			}

			++pageIndex;
		}

		MenuControllerComp.TotalPages = pageIndex;
	}



	REGISTER_SCENE_AS(MenuScene, MENU_SCENE_NAME);

}
