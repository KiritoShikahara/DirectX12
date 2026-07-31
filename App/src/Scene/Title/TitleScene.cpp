#include"apppch.h"
#include "TitleScene.h"
#include<ecs/system/manager/ComponentSystemManager.h>

#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/TitleInputSystem/TitleInputSystem.h>
#include<system/UI/UiPanelUtility.h>
#include<graphics/Text/Renderer/TextRenderer.h>

#include"../macros.h"

namespace scene
{
	void TitleScene::Initialize()
	{
		// TimeScale縺ｯ繝励Ο繧ｻ繧ｹ蜈ｨ菴薙〒蜈ｱ譛峨＆繧後√す繝ｼ繝ｳ繧定ｷｨ縺・〒繧よ戟縺｡雜翫＆繧後ｋ縲・
		// GameOver/PerkSelect遲峨〒TimeScale=0.0縺ｮ縺ｾ縺ｾResult竊探itle縺ｸ驕ｷ遘ｻ縺励※縺上ｋ繧ｱ繝ｼ繧ｹ縺後≠繧九◆繧√・
		// 荳譎ょ●豁｢縺ｮ讎ょｿｵ縺檎┌縺Уitle縺ｧ縺ｯ蠢・★1.0縺ｸ謌ｻ縺・
		// ・域ｷｱ縺乗ｭ｢繧√ｋ縺ｨGlowAnimation遲詠awDeltaTime萓晏ｭ倥・貍泌・縺励°蜍輔°縺ｪ縺上↑繧具ｼ峨・
		GetTime().SetTimeScale(1.0);

		CreateCompSystem();

		LoadResource();
		CreateBackground();
		CreateLogo();
		CreatePromptText();
		CreateControlGuide();

		DEBUG_LOG(::sys::eLogLevel::Log, "Title Scene.");
	}

	void TitleScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();
		::audio::AudioManager::Get().ClearSceneSounds();
	}

	void TitleScene::CreateCompSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();

		// 閭梧勹轤ｹ貊・
		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);

		// 蜈･蜉・
		manager.AddUserSystem<::sys::TitleInputSystem>(::ecs::eUpdatePhase::PostUpdate);
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

		// 髻ｳ讌ｽ
		PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7);

	}

	void TitleScene::CreateLogo()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& window = ::sys::Window::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_Logo.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		trans.Set2DPosition(window.GetVirtualWidth() / 2, window.GetVirtualHeight() / 5 * 2);

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
		glow.Frequency = 0.7;
		glow.PhaseOffset = 0.1;

	}

	void TitleScene::CreatePromptText()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& window = ::sys::Window::Get();
		auto entity = manager.CreateEntity();
		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_Prompt.png");

		auto& trans = manager.AddComponent<::ecs::Transform>(entity);
		trans.Set2DPosition(window.GetVirtualWidth() / 2, window.GetVirtualHeight() / 5 * 4);

		float scale = 0.6f;
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Size = { 1920,1080 };
		sprite.DrawScale = { scale ,scale };
		sprite.Intensity = 1.0f;
		sprite.Pivot = { 0.5,0.5 };
		sprite.SetLayer(::ecs::SpriteLayer::Character);

		auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
		glow.Amplitude = 0.5;
		glow.BaseIntensity = 1.2;
		glow.Frequency = 1.5;
		glow.PhaseOffset = 0;

	}

	void TitleScene::CreateControlGuide()
	{
		// 縲訓USH TO START縲咲判蜒・蝗ｺ螳壹・闍ｱ隱樒判蜒上〒繝懊ち繝ｳ蜷阪∪縺ｧ縺ｯ遉ｺ縺帙↑縺・縺ｮ荳九↓縲・
		// 螳滄圀縺ｫ謚ｼ縺吶∋縺阪・繧ｿ繝ｳ蜷阪ｒ陦ｨ遉ｺ縺吶ｋ縲ゅ郡elect縺｣縺ｦ菴輔・繧ｿ繝ｳ・溘阪→縺ｪ繧峨↑縺・ｈ縺・・
		// 蜈･蜉帙ョ繝舌う繧ｹ縺ｫ蠢懊§縺溷・螳ｹ(Space/A繝懊ち繝ｳ遲・縺ｸTitleInputSystem縺梧ｯ弱ヵ繝ｬ繝ｼ繝譖ｴ譁ｰ縺吶ｋ
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& window = ::sys::Window::Get();

		auto& textRenderer = ::graphics::TextRenderer::Get();

		// guideCenterYは文字とパネルの見た目上の縦中心に置きたい座標。
		// TextComponent::Yはベースライン座標(グリフはそこから上下非対称に広がる)なので、
		// そのままguideCenterYを渡すと中心がずれる(実際に発生した不具合)。
		// MeasureVerticalCenterOffsetでベースラインYへ変換する。
		const float guideCenterY = static_cast<float>(window.GetVirtualHeight()) / 5.0f * 4.0f + 90.0f;
		constexpr float kTextSize = 30.0f;

		// 閭梧勹(繧ｿ繧､繝医Ν逕ｻ蜒・縺ｮ荳翫↓逶ｴ謗･荵励ｋ縺ｨ隱ｭ縺ｿ縺･繧峨＞縺溘ａ縲・ｻ貞濠騾乗・縺ｮ譚ｿ繧剃ｸ九↓謨ｷ縺・
		// (PerkSelectSystem遲峨→蜷後§謇区ｳ輔ょ・騾壼喧縺ｯUiPanelUtility蜿ら・)
		constexpr float kGuidePanelWidth = 260.0f;
		constexpr float kGuidePanelPadY = 16.0f;
		::ecs::uiutil::CreateTranslucentPanel(
			static_cast<float>(window.GetVirtualWidth()) * 0.5f,
			guideCenterY,
			kGuidePanelWidth,
			kTextSize + kGuidePanelPadY * 2.0f,
			0);

		auto entity = manager.CreateEntity();
		auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
		text.Y = guideCenterY + textRenderer.MeasureVerticalCenterOffset(kTextSize);
		text.Size = kTextSize;
		text.Color = { 0.85f, 0.9f, 1.0f, 1.0f };
		text.Layer = 10;

		registry.emplace<::ecs::TitleGuideUiTag>(entity);
	}

	REGISTER_SCENE_AS(TitleScene, TITLE_SCENE_NAME);
}

