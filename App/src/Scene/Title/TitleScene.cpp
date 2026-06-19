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
			manager.GetOrLoad("Assets/Texture/BackGround/TX_TitleBG.png");

		}
	}

	void TitleScene::CreateBackground()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/BackGround/TX_TitleBG.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920,1080 };
		sprite.Intensity = 1.0f;

		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 2.5f;
		glow.BaseIntensity = 5;
		glow.Frequency = 0.7;
		glow.PhaseOffset = 0.0f;
	}

	void TitleScene::CreateLogo()
	{
	}

	void TitleScene::CreatePromptText()
	{
	}

	REGISTER_SCENE_AS(TitleScene, "Title");
}

