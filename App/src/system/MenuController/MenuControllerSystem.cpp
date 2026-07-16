#include "apppch.h"
#include "MenuControllerSystem.h"

#include"MenuControllerComp.h"
#include<Scene/Hub/HubScene.h>
#include<Scene/Game/GameScene.h>

#include"../macros.h"

namespace ecs
{
	void MenuSlideSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		registry.view<Transform, MenuSlideComp>().each(
			[deltaTime](Transform& transform, MenuSlideComp& slide)
			{
				const DirectX::XMFLOAT2 pos = transform.Get2DPosition();

				const float t = std::clamp(slide.SlideSpeed * deltaTime, 0.0f, 1.0f);
				const float newX = pos.x + (slide.TargetX - pos.x) * t;

				transform.Set2DPosition(newX, pos.y);
			});

	}
	
	void MenuPagingSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto controllerView = registry.view<MenuControllerComp>();
		if (controllerView.size() <= 0)
		{
			return;
		}

		auto& controller = controllerView.get<MenuControllerComp>(controllerView.front());
		const float centerX = controller.WindowWidth * 0.5f;

		registry.view<SpellMenuDataComp, MenuSlideComp>().each(
			[&controller, centerX](const SpellMenuDataComp& spellData, MenuSlideComp& slide)
			{
				const float diff = static_cast<float>(spellData.PageIndex) - static_cast<float>(controller.CurrentlySelectedIdx);
				slide.TargetX = centerX + diff * controller.WindowWidth;

				if (spellData.PageIndex == controller.CurrentlySelectedIdx)
				{
					controller.ActiveSpellID = spellData.SpellID;
				}
			});

	}

	void MenuInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto controllerView = registry.view<MenuControllerComp>();
		if (controllerView.begin() == controllerView.end())
		{
			return;
		}

		auto& controller = controllerView.get<MenuControllerComp>(controllerView.front());

		if (controller.TotalPages == 0)
		{
			return;
		}

		const bool pressedRight = sys::InputManager::Get().IsActionPressed("MenuRight");
		const bool pressedLeft = sys::InputManager::Get().IsActionPressed("MenuLeft");

		if (pressedRight)
		{
			controller.CurrentlySelectedIdx = (controller.CurrentlySelectedIdx + 1) % controller.TotalPages;
		}
		else if (pressedLeft)
		{
			controller.CurrentlySelectedIdx = (controller.CurrentlySelectedIdx + controller.TotalPages - 1) % controller.TotalPages;
		}
	}


	void MenuSelectInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto& input = ::sys::InputManager::Get();

		// セレクトならゲーム
		if (input.IsActionPressed("Select") == true)
		{
			auto controllerView = registry.view<MenuControllerComp>();
			if (controllerView.begin() == controllerView.end())
			{
				return;
			}
			auto& controller = controllerView.get<MenuControllerComp>(controllerView.front());

			::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::GameScene>(::sys::FadeOptions{},controller.ActiveSpellID);
			PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
			return;
		}

		// 戻るならハブ画面（タイトルへ直接戻すと2択の前段が飛ばされてしまうため）
		if (input.IsActionPressed("Cancel") == true)
		{
			::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::HubScene>();
			PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
			return;
		}
	}
}