#include "apppch.h"
#include "MenuControllerSystem.h"

#include"MenuControllerComp.h"
#include"WeaponSelectVisuals.h"
#include<graphics/Text/Renderer/TextRenderer.h> // テキストの水平中央揃えTargetXの計算に使う
#include<system/Input/InputGuideLabels.h>
#include<Scene/Hub/HubScene.h>
#include<Scene/Game/GameScene.h>

#include"../macros.h"

namespace ecs
{
	void MenuSlideSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// Sprite/Shape(円・アイコン等、Transformで座標を持つ要素)
		registry.view<Transform, MenuSlideComp>().each(
			[deltaTime](Transform& transform, MenuSlideComp& slide)
			{
				const DirectX::XMFLOAT2 pos = transform.Get2DPosition();

				const float t = std::clamp(slide.SlideSpeed * deltaTime, 0.0f, 1.0f);
				const float newX = pos.x + (slide.TargetX - pos.x) * t;

				transform.Set2DPosition(newX, pos.y);
			});

		// ラベル等のテキスト要素。TextComponentはTransformを使わずX/Yを直接持つため
		// (TextComponent::Xのコメント参照)、Transform版とは別に同じ補間をここで行う。
		// これが無いとページ送りでアイコンだけスライドしてラベルが取り残される。
		registry.view<TextComponent, MenuSlideComp>().each(
			[deltaTime](TextComponent& text, MenuSlideComp& slide)
			{
				const float t = std::clamp(slide.SlideSpeed * deltaTime, 0.0f, 1.0f);
				text.X = text.X + (slide.TargetX - text.X) * t;
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

		// Sprite/Shape(円・アイコン等)。ページ中心のX座標そのものがTargetXでよい
		// (TextComponentを持つエンティティは下の別ループで処理するため、ここでは除外する。
		// 除外しないと、テキストの水平中央揃えオフセット(-文字幅/2)がここで上書きされ、
		// 毎フレーム位置がズレていくバグになる)。
		registry.view<SpellMenuDataComp, MenuSlideComp>(entt::exclude<TextComponent>).each(
			[&controller, centerX](const SpellMenuDataComp& spellData, MenuSlideComp& slide)
			{
				const float diff = static_cast<float>(spellData.PageIndex) - static_cast<float>(controller.CurrentlySelectedIdx);
				slide.TargetX = centerX + diff * controller.WindowWidth;

				if (spellData.PageIndex == controller.CurrentlySelectedIdx)
				{
					controller.ActiveSpellID = spellData.SpellID;
				}
			});

		// ラベル・説明文等のテキスト要素。文字幅の半分だけ左にずらした位置が
		// 水平中央揃えのTargetXになる(MenuScene::CreateSpellsの初期配置計算と一致させる)。
		auto& textRenderer = ::graphics::TextRenderer::Get();
		registry.view<SpellMenuDataComp, MenuSlideComp, TextComponent>().each(
			[&controller, centerX, &textRenderer](const SpellMenuDataComp& spellData, MenuSlideComp& slide, const TextComponent& text)
			{
				const float diff = static_cast<float>(spellData.PageIndex) - static_cast<float>(controller.CurrentlySelectedIdx);
				const float restX = centerX + diff * controller.WindowWidth;
				const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
				slide.TargetX = restX - textWidth * 0.5f;

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

		// 操作案内。最後に使われた入力デバイスに応じてボタン表示名を切り替える
		{
			const ::sys::eInputDevice device = ::sys::InputManager::Get().GetLastInputDevice();
			// text.Xは中央揃えのため毎フレーム書き換えられる値なので、基準はここで都度
			// 画面幅から求める(text.Xを基準にすると書き換え後の値を元に再計算してしまい、
			// 位置がズレ続けるバグになる。StatusUpgradeCardUiTag::CenterXと同じ注意点)。
			const float screenCenterX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;
			auto& textRenderer = ::graphics::TextRenderer::Get();

			registry.view<MenuGuideUiTag, TextComponent>().each(
				[device, screenCenterX, &textRenderer](TextComponent& text)
				{
					text.Text = std::wstring(ecs::inputguide::GetMenuMoveLabel(device)) + L":選択　" +
						ecs::inputguide::GetSelectLabel(device) + L":決定　" +
						ecs::inputguide::GetCancelLabel(device) + L":ハブへ戻る";

					const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
					text.X = screenCenterX - textWidth * 0.5f;
				});
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