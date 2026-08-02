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

		// スレッドにはshared_ptr/std::functionのコピーのみを渡し、thisやレジストリへの参照は渡さない
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

		// 二重joinや未起動を防ぐためjoinable判定してからjoinする
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

		// 背景。白テクスチャを黒着色して全画面を覆う、他画面のパネルと同じ手法
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

		// スピナー。LoadingScreenUpdateSystemが毎フレーム回転させる
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

		// Loadingテキスト。中央揃え
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

		// 進捗バー。枠と塗りの2枚構成で、塗りはSprite::FillAmountで左から右へ埋める
		{
			const float kBarWidth = static_cast<float>(window.GetVirtualWidth()) * 0.6f;
			constexpr float kBarHeight = 32.0f;
			const float barY = static_cast<float>(window.GetVirtualHeight()) * 0.88f;

			auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Effect/Texture/White.png");

			// 枠、背景
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

			// 塗り、進捗に応じてFillAmountが更新される
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

			// パーセントテキスト、バーの下
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
