#include "pch.h"
#include "Engine.h"

#include<system/Window/Window.h>
#include<system/Engine/EngineContext.h>

#include<graphics/Dx12/Dx12Device.h>
#include<system/Logger/Logger.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<ecs/entity/EntityManager.h>

namespace sys
{

	Engine::Engine()
		: mIsRunning(false)
		, mIsInitialized(false)
		, mWindow(nullptr)
		, mDevice(nullptr)
		, mRenderer(nullptr)
		, mImGuiManager(nullptr)
	{
	}

	/// <summary>
	/// App初期化
	/// </summary>
	/// <param name="context">初期化情報</param>
	/// <returns>true:成功 false:失敗</returns>
	bool Engine::Initialize(EngineContext context)
	{
		// Loggerの初期化
		if (sys::Logger::Get().Initialize() == false)
		{
			return false;
		}

		// ウィンドウの初期化
		mWindow = &Window::Get();
		if(mWindow->Initialize(context.WindowContext) == false)
		{
			return false;
		}

		// DX12デバイス初期化
		mDevice = &graphics::DX12Device::Get();
		if (mDevice->Initialize() == false)
		{
			return false;
		}

		// DX12描画管理クラス初期化
		mRenderer = std::make_unique<graphics::DX12Renderer>();
		if (mRenderer->Initialize(
			mDevice, 
			mWindow->GetHWND(), 
			mWindow->GetWidth(), 
			mWindow->GetHeight()
		) == false)
		{
			return false;
		}

		// GraphicsDescriptorHeapManagerの初期化
		auto& descriptorHeapManager = graphics::GDescriptorHeapManager::Get();
		if(descriptorHeapManager.Initialize(mDevice->GetDevice()) == false)
		{
			return false;
		}

		// ImGuiManagerの初期化
		mImGuiManager = &sys::ImGuiManager::Get();
		if (mImGuiManager->Initialize(*mWindow, *mDevice, *mRenderer, descriptorHeapManager) == false)
		{
			return false;
		}

		// EntityManagerの初期化
		mEntityManager = &ecs::EntityManager::Get();
		if (mEntityManager->Initialize() == false)
		{
			return false;
		}

		mIsRunning = true;
		mIsInitialized = true;

		DEBUG_LOG(sys::eLogLevel::Log, "Engine initialized successfully.");

		return true;
	}

	/// <summary>
	/// Appの実行
	/// </summary>
	bool Engine::Run()
	{
		if (mIsInitialized == false)  return false;

		// OSメッセ処理
		mWindow->ProcessMessages();

		if (mWindow->IsQuitRequested())
		{
			mIsRunning = false;
			return false;
		}

		// TODO:更新処理

		// TODO:描画処理
		mRenderer->BeginRendering();
		mRenderer->SetViewPort(
			static_cast<float>(mWindow->GetWidth()),
			static_cast<float>(mWindow->GetHeight()));

		mImGuiManager->NewFrame();
		mImGuiManager->Update();
		Render();

		mImGuiManager->EndFrame();
		mRenderer->Flip();

		// TODO:フレームの終了処理

		return true;
	}

	/// <summary>
	/// App終了処理
	/// </summary>
	/// <returns>true:成功 false:失敗</returns>
	bool Engine::Finalize()
	{
		if (mIsInitialized == false)  return false;

		// GPU完了待ち
		if (mRenderer != nullptr)
		{
			mRenderer->WaitForGPU();
		}

		// Dx12Rendererの破棄
		mRenderer->Finalize();
		mRenderer.reset();

		// Dx12Deviceの破棄
		mDevice->Finalize();
		mDevice = nullptr;

		// TODO:ログ出力

		sys::Logger::Get().Finalize();

		return true;
	}

	/// <summary>
	/// 描画
	/// </summary>
	void Engine::Render()
	{


	}
}
