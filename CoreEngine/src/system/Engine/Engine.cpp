#include "pch.h"
#include "Engine.h"

#include<system/Window/Window.h>
#include<system/Engine/EngineContext.h>

#include<graphics/Dx12/Dx12Device.h>

namespace sys
{

	Engine::Engine()
		: mIsRunning(false)
		, mIsInitialized(false)
		, mWindow(nullptr)
		, mDevice(nullptr)
		, mRenderer(nullptr)
	{
	}

	/// <summary>
	/// App初期化
	/// </summary>
	/// <param name="context">初期化情報</param>
	/// <returns>true:成功 false:失敗</returns>
	bool Engine::Initialize(EngineContext context)
	{
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

		mIsRunning = true;
		mIsInitialized = true;

		// TODO:ログ出力

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

		Render();

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
		return true;
	}

	/// <summary>
	/// 描画
	/// </summary>
	void Engine::Render()
	{


	}
}
