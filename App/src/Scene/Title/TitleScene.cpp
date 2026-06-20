#include"apppch.h"
#include "TitleScene.h"
#include<ecs/system/manager/ComponentSystemManager.h>

#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>

namespace scene
{
	void TitleScene::Initialize()
	{
		CreateCompSystem();

		LoadResource();
		CreateBackground();
		CreateLogo();
		CreatePromptText();
	}

	void TitleScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
	}

	void TitleScene::CreateCompSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();

		// 背景点滅
		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);
	}

	void TitleScene::LoadResource()
	{
		{
			auto& manager = graphics::TextureManager::Get();
			manager.GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");
			manager.GetOrLoad("Assets/Texture/Title/TX_Logo.png");
		}
	}

	void TitleScene::CreateBackground()
	{
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
		glow.Frequency = 0.7;
		glow.PhaseOffset = 0.0f;

	}

	void TitleScene::CreateLogo()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_Logo.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		trans.Set2DPosition(sys::Window::Get().GetVirtualWidth() / 2, sys::Window::Get().GetVirtualHeight() / 2);

		float scale = 0.8f;
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920,1080 };
		sprite.DrawScale = { scale ,scale };
		sprite.Intensity = 1.0f;
		sprite.Pivot = { 0.5,0.5 };
		sprite.SetLayer(::ecs::SpriteLayer::Character);

		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 1.0;
		glow.BaseIntensity =2;
		glow.Frequency = 0.5;
		glow.PhaseOffset = 0.0;

	}

	void TitleScene::CreatePromptText()
	{
	}

	REGISTER_SCENE_AS(TitleScene, "Title");
}

