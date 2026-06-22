#include "apppch.h"
#include "MenuScene.h"
#include"../macros.h"

namespace scene
{
	void MenuScene::Initialize()
	{
		CreateBG();
		DEBUG_LOG(::sys::eLogLevel::Log, "Menu Scene.");
	}

	void MenuScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
		::audio::AudioManager::Get().ClearSceneSounds();
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
