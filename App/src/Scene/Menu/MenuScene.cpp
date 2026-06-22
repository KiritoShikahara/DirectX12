#include "apppch.h"
#include "MenuScene.h"
#include"../macros.h"

#include<Data/Menu/MenuSpellsData.h>

namespace scene
{
	void MenuScene::Initialize()
	{
		LoadData();
		CreateBG();
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

		auto data = dataReg.GetManager<::data::SpellMenuData>().GetAll();
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

		// 音楽
	}

	REGISTER_SCENE_AS(MenuScene, MENU_SCENE_NAME);

}
