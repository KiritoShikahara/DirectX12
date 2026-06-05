#include "pch.h"
#include "Engine.h"

#include<system/Window/Window.h>
#include<system/Engine/EngineContext.h>

#include<system/Input/InputManager.h>

// Dx12
#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Dx12/Dx12Context.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<graphics/Dx12/RenderContext.h>

#include<system/Logger/Logger.h>
#include<system/Camera/CameraSystem.h>

// Fbx
#include<graphics/Fbx/Renderer/FbxRenderer.h>
#include<graphics/Fbx/Animation/FbxAnimSystem.h>

// 2D
#include<graphics/Shader/ShaderManager.h>
#include<graphics/Texture/TextureManager.h>
#include<graphics/Sprite/Renderer/SpriteRenderer.h>

// Component
#include<ecs/entity/EntityManager.h>
#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/camera/CameraComponent.h>
#include<ecs/component/Fbx/FbxComponent.h>
#include<ecs/component/Fbx/FbxAnimComponent.h>

// Resoruce
#include<graphics/Texture/Texture.h>
#include<graphics/Fbx/Resource/FbxResourceManager.h>
#include<graphics/Fbx/Resource/FbxResource.h>

// テスト用のSpriteの作成
void SpriteRenderTest()
{
	auto texture = graphics::TextureManager::Get().GetOrLoad("Assets/Test/test.png");
	auto entity = ecs::EntityManager::Get().CreateEntity();
	auto& tr = ecs::EntityManager::Get().AddComponent<ecs::Transform>(entity);
	auto& sprite = ecs::EntityManager::Get().AddComponent<ecs::Sprite>(entity,texture);
}

// テスト用のリソース読み込み
void LoadResource()
{
	auto& manager = graphics::FbxResourceManager::Get();
	auto res = manager.Load("Assets/Fbx/Faul.fbx.bin");
	bool ret = manager.LoadAnm("Assets/Fbx/Faul.fbx.bin", "Assets/Fbx/Animation/Attack_A.fbx.anm", "Attack_A");
}


// テスト用のFBXモデルの作成
void Create3DModel()
{
	auto& manager = ecs::EntityManager::Get();
	auto& reg = manager.GetRegistry();

	auto res = graphics::FbxResourceManager::Get().Load("Assets/Fbx/Faul.fbx.bin");

	float scale = 0.2f;

	auto entity = manager.CreateEntity();
	auto& tr = manager.AddComponent<ecs::Transform>(entity);
	tr.SetScale(scale);
	tr.SetEulerAnglesDeg(-90, -90, -90);

	auto& fbx = manager.AddComponent<ecs::FbxComponent>(entity);
	fbx.Resource = res;

	auto& anim = manager.AddComponent<ecs::FbxAnimComponent>(entity);
	anim.Play(*fbx.Resource, "Attack_A", true);

}

// テスト用のカメラ作成
entt::entity CreateCamera()
{
	auto& registry = ecs::EntityManager::Get().GetRegistry();

	entt::entity entity = ecs::EntityManager::Get().CreateEntity();

	auto& tr = registry.emplace<ecs::Transform>(entity);

	tr.SetPosition(0.0f, 1.0f, -100.0f);

	// もし Transform クラスに回転を設定する関数（SetRotation や LookAt）があれば、
	// ここで「正面（(0,0,0) 方向）を向く」ように回転をリセット、または設定してください。
	// 例: tr.SetRotation(0.0f, 0.0f, 0.0f); 

	auto& cam = registry.emplace<ecs::CameraComponent>(entity);
	cam.IsMainCamera = true;
	cam.Fov = 60.0f;
	cam.Near = 0.1f;  // 近すぎてクリップするのを防ぐため 0.01f から 0.1f に推奨変更
	cam.Far = 1000.0f;
	cam.SetAspectRatioFromWindow(sys::Window::Get());

	return entity;
}

namespace sys
{

	Engine::Engine()
		: mIsRunning(false)
		, mIsInitialized(false)
		, mWindow(nullptr)
		, mDevice(nullptr)
		, mImGuiManager(nullptr)
		, mInputManager(nullptr)
		, mDX12Renderer(nullptr)
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

		// Timeの初期化
		mTime.Initialize();

		// ウィンドウの初期化
		mWindow = &Window::Get();
		if (mWindow->Initialize(context.WindowContext) == false)
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
		mDX12Renderer = &graphics::DX12Renderer::Get();
		if (mDX12Renderer->Initialize(
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
		if (descriptorHeapManager.Initialize(mDevice->GetDevice()) == false)
		{
			return false;
		}

		// ImGuiManagerの初期化
		mImGuiManager = &sys::ImGuiManager::Get();
		if (mImGuiManager->Initialize(*mWindow, *mDevice, *mDX12Renderer->GetContext(), descriptorHeapManager) == false)
		{
			return false;
		}

		// EntityManagerの初期化
		mEntityManager = &ecs::EntityManager::Get();
		if (mEntityManager->Initialize() == false)
		{
			return false;
		}

		mInputManager = &sys::InputManager::Get();
		if (mInputManager->Initialize() == false)
		{
			return false;
		}

		// AssetsPath
		SINGLETON_REF(sys::AssetPathManager, AssetManager);
		AssetManager.Initialize();

		// TextureManager
		SINGLETON_REF(graphics::TextureManager, TextureManager);

		// Renderer
		SINGLETON_REF(graphics::SpriteRenderer, SpriteRenderer);
		if(SpriteRenderer.Initialize(*mDevice, descriptorHeapManager, graphics::ShaderManager::Get(), *mWindow) == false)
		{
			return false;
		}

		// Fbx
		SINGLETON_REF(graphics::FbxRenderer, FbxRenderer);
		if (FbxRenderer.Initialize(*mDevice, descriptorHeapManager, graphics::ShaderManager::Get()) == false)
		{
			return false;
		}

		// カメラ
		if (sys::CameraSystem::Get().Initialize() == false)
		{
			return false;
		}




		// テスト用のインスタンス生成
		CreateCamera();
		LoadResource();
		Create3DModel();

		//SpriteRenderTest();


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
		// 事前更新
		this->PreUpdate();

		// メイン更新
		this->Update();

		// 事後更新
		this->PostUpdate();



		auto context = mDX12Renderer->GetContext();

		// TODO:描画処理
		mDX12Renderer->BeginFrame();
		graphics::RenderContext::Get().SetFrameIndex(context->GetCurrentFrameIndex());

		mImGuiManager->NewFrame();
		mImGuiManager->Update();
		Render();

		mImGuiManager->EndFrame();
		mDX12Renderer->EndFrame();

		// TODO:フレームの終了処理
		mInputManager->Update();

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
		if (mDX12Renderer != nullptr)
		{
			mDX12Renderer->WaitForGPU();
		}

		// Dx12Rendererの破棄
		mDX12Renderer->Finalize();
		mDX12Renderer = nullptr;

		// Dx12Deviceの破棄
		mDevice->Finalize();
		mDevice = nullptr;

		// TODO:ログ出力

		sys::Logger::Get().Finalize();

		return true;
	}


	/// <summary>
	/// 事前更新
	/// </summary>
	void Engine::PreUpdate()
	{
		// Timeの更新
		mTime.Update();

	}

	/// <summary>
	/// 状態更新
	/// </summary>
	void Engine::Update()
	{
		if (INPUT_PAD->IsPressed(sys::ePadButton::R2))
		{
			std::cout << "Push" << std::endl;
		}
	}

	/// <summary>
	/// 事後更新
	/// </summary>
	void Engine::PostUpdate()
	{
		auto& registry = ecs::EntityManager::Get().GetRegistry();
		auto dt = mTime.GetDeltaTime();

		// カメラ行列の更新
		sys::CameraSystem::Get().Update(registry);

		// アニメーション時間の更新
		graphics::FbxAnimSystem::Update(registry, dt);
	}

	/// <summary>
	/// 描画
	/// </summary>
	void Engine::Render()
	{
		auto context = mDX12Renderer->GetContext();

		auto& registry = mEntityManager->GetRegistry();
		auto cmdList = context->GetCommandList();

		// 3Dモデル
		SINGLETON_REF(graphics::FbxRenderer, FbxRenderer);
		FbxRenderer.Begin();
		FbxRenderer.UpdateAndDraw(registry);
		FbxRenderer.End(cmdList);

		// 2DSprite
		SINGLETON_REF(graphics::SpriteRenderer, spriteRenderer);
		spriteRenderer.Begin();
		spriteRenderer.UpdateAndDraw(registry);
		spriteRenderer.End(cmdList);
	}
}
