#include "apppch.h"
#include "LoadingScene.h"

#include<ecs/system/manager/ComponentSystemManager.h>
#include<system/Loading/LoadingScreenComponent.h>
#include<system/Loading/LoadingScreenUpdateSystem.h>
#include<graphics/Text/Renderer/TextRenderer.h>

#include"../macros.h"

namespace scene
{
	LoadingScene::LoadingScene(
		std::function<void(::ecs::LoadingProgress&)> preloadFn,
		std::function<void(::ecs::LoadingProgress&)> resolveFn,
		std::function<void()> onComplete)
		: mPreloadFn(std::move(preloadFn))
		, mResolveFn(std::move(resolveFn))
		, mOnComplete(std::move(onComplete))
	{
	}

	void LoadingScene::Initialize()
	{
		::ecs::ComponentSystemManager::Get().AddUserSystem<::ecs::LoadingScreenUpdateSystem>(::ecs::eUpdatePhase::Update);

		CreateLoadingUi();

		// 完了フラグを持つコントローラーエンティティを生成し、バックグラウンドスレッドで
		// preloadFn(CPU専用処理)を実行する。スレッドへ渡すのはshared_ptr/std::functionの
		// コピーのみで、thisやレジストリへの参照は渡さない(LoadingScene自体の生存期間・
		// スレッド安全性に依存しないようにするため)。resolveFn(GPUリソース生成)は
		// このスレッドからは呼ばず、LoadingScreenUpdateSystemがIsCpuDoneを検知した時点で
		// メインスレッドから呼ぶ(LoadingScreenComponentのコメント参照)。
		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto& loading = manager.AddComponent<::ecs::LoadingScreenComponent>(entity);
		loading.ResolveFn = mResolveFn;
		loading.OnComplete = mOnComplete;

		auto cpuDoneFlag = loading.IsCpuDone;
		auto progress = loading.Progress;
		auto preloadFn = mPreloadFn;
		mLoadingThread = std::thread([preloadFn, cpuDoneFlag, progress]()
			{
				if (preloadFn) preloadFn(*progress);
				cpuDoneFlag->store(true, std::memory_order_release);
			});

		DEBUG_LOG(::sys::eLogLevel::Log, "Loading Scene.");
	}

	void LoadingScene::Finalize()
	{
		::ecs::ComponentSystemManager::Get().ClearUserSystems();

		// preloadFn完了(IsCpuDone==true)を確認してからOnComplete経由でシーン切り替えを
		// 要求する設計のため、ここに来る時点でスレッドは完了しているか完了間際のはずだが、
		// 念のため明示的にjoinしてから終了する(joinable()チェックで二重join/未起動を防ぐ)。
		if (mLoadingThread.joinable())
		{
			mLoadingThread.join();
		}
	}

	void LoadingScene::CreateLoadingUi()
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto& registry = ENTT_REGISTRY;
		auto& window = ::sys::Window::Get();

		const float centerX = static_cast<float>(window.GetVirtualWidth()) * 0.5f;
		const float centerY = static_cast<float>(window.GetVirtualHeight()) * 0.5f;

		// 背景(全画面の黒フィル。白テクスチャを黒着色して流用する、他画面のパネルと同じ手法)
		{
			auto entity = manager.CreateEntity();
			auto& transform = manager.AddComponent<::ecs::Transform>(entity);
			transform.Set2DPosition(centerX, centerY);

			auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Effect/Texture/White.png");
			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
			sprite.Pivot = { 0.5f, 0.5f };
			sprite.Size = { static_cast<float>(window.GetVirtualWidth()), static_cast<float>(window.GetVirtualHeight()) };
			sprite.Color = ::graphics::Color(0.0f, 0.0f, 0.0f, 1.0f);
			sprite.SetLayer(::ecs::SpriteLayer::Background);
		}

		// スピナー(回転アイコン、LoadingScreenUpdateSystemが毎フレーム回転させる)
		{
			constexpr float kSpinnerSize = 96.0f;

			auto entity = manager.CreateEntity();
			auto& transform = manager.AddComponent<::ecs::Transform>(entity);
			transform.Set2DPosition(centerX, centerY - 40.0f);

			auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Icon/loading.png");
			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
			sprite.Pivot = { 0.5f, 0.5f };
			sprite.Size = { kSpinnerSize, kSpinnerSize };
			sprite.SetLayer(::ecs::SpriteLayer::UI, 0);

			registry.emplace<::ecs::LoadingSpinnerUiTag>(entity);
		}

		// "Loading..."テキスト(中央揃え)
		{
			constexpr float kTextSize = 32.0f;
			const std::wstring label = L"Loading...";

			auto& textRenderer = ::graphics::TextRenderer::Get();
			const float textWidth = textRenderer.MeasureWidth(label, kTextSize);

			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = label;
			text.X = centerX - textWidth * 0.5f;
			text.Y = centerY + 60.0f;
			text.Size = kTextSize;
			text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			text.Layer = 10;
		}

		// 進捗バー(枠+塗り。塗りはSprite::FillAmountで左から右へ塗り進める。
		// FillAmountはUV空間でのクリップなのでSize/位置は枠と同一で構わない)。
		// 画面下部に大きく表示する(視認性優先。横幅は画面幅基準の可変値にする)。
		{
			const float kBarWidth = static_cast<float>(window.GetVirtualWidth()) * 0.6f;
			constexpr float kBarHeight = 32.0f;
			const float barY = static_cast<float>(window.GetVirtualHeight()) * 0.88f;

			auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Effect/Texture/White.png");

			// 枠(背景)
			{
				auto entity = manager.CreateEntity();
				auto& transform = manager.AddComponent<::ecs::Transform>(entity);
				transform.Set2DPosition(centerX, barY);

				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { kBarWidth, kBarHeight };
				sprite.Color = ::graphics::Color(0.3f, 0.3f, 0.3f, 1.0f);
				sprite.SetLayer(::ecs::SpriteLayer::UI, 0);
			}

			// 塗り(進捗に応じてFillAmountが0-1で更新される)
			{
				auto entity = manager.CreateEntity();
				auto& transform = manager.AddComponent<::ecs::Transform>(entity);
				transform.Set2DPosition(centerX, barY);

				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { kBarWidth, kBarHeight };
				sprite.Color = ::graphics::Color(1.0f, 1.0f, 1.0f, 1.0f);
				sprite.FillAmount = 0.0f;
				sprite.SetLayer(::ecs::SpriteLayer::UI, 1);

				registry.emplace<::ecs::LoadingProgressBarFillTag>(entity);
			}

			// パーセントテキスト(バーの下)
			{
				constexpr float kPercentTextSize = 36.0f;
				const std::wstring label = L"0%";

				auto& textRenderer = ::graphics::TextRenderer::Get();
				const float textWidth = textRenderer.MeasureWidth(label, kPercentTextSize);

				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
				text.Text = label;
				text.X = centerX - textWidth * 0.5f;
				text.Y = barY + kBarHeight * 0.5f + 16.0f;
				text.Size = kPercentTextSize;
				text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
				text.Layer = 10;

				registry.emplace<::ecs::LoadingProgressTextTag>(entity);
			}
		}
	}
}
