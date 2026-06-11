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

// Audio
#include<audio/Device/AudioDevice.h>
#include<audio/Manager/AudioManager.h>
#include<audio/Resource/AudioResourceManager.h>

// 3D
#include<graphics/Fbx/Renderer/FbxRenderer.h>
#include<graphics/Fbx/Animation/FbxAnimSystem.h>
#include<graphics/PrimitiveModel/Resource/PrimitiveResourceManager.h>

// Camera
#include<system/Camera/CameraSystem.h>

// Light
#include<system/Light/LightSystem.h>

// 2D
#include<graphics/Shader/ShaderManager.h>
#include<graphics/Texture/TextureManager.h>
#include<graphics/Sprite/Renderer/SpriteRenderer.h>

// ECS
#include<ecs/entity/EntityManager.h>
#include<ecs/system/manager/ComponentSystemManager.h>

// Phisics
#include<system/Physics/System/PhysicsSystem.h>
#include<system/Physics/Manager/PhysicsManager.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>

// Scene
#include<system/Scene/Manager/SceneManager.h>
#include<system/Scene/Factory/SceneFactory.h>

// Component
#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/camera/CameraComponent.h>
#include<ecs/component/Fbx/FbxComponent.h>
#include<ecs/component/Fbx/FbxAnimComponent.h>
#include<ecs/component/Light/LightComponent.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>

// Resoruce
#include<graphics/Texture/Texture.h>
#include<graphics/Fbx/Resource/FbxResourceManager.h>
#include<graphics/Fbx/Resource/FbxResource.h>


// define
#include"EngineDefine.h"


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
	// fbx
	{
		auto& manager = graphics::FbxResourceManager::Get();
		auto res = manager.Load("Assets/Fbx/Faul.fbx.bin");
		bool ret = manager.LoadAnm("Assets/Fbx/Faul.fbx.bin", "Assets/Fbx/Animation/Attack_A.fbx.anm", "Attack_A");
		ret = manager.LoadAnm("Assets/Fbx/Faul.fbx.bin", "Assets/Fbx/Animation/Attack_B.fbx.anm", "Attack_B");
	}

	// audio
	{
		auto& ResManager = audio::AudioResourceManager::Get();
		auto res = ResManager.GetResource("Assets/SE/TestSE.aud");
	}

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
	tr.SetPosition(0, 10, 0);

	auto& fbx = manager.AddComponent<ecs::FbxComponent>(entity);
	fbx.Resource = res;
	fbx.CustomColor = { 1,1,1,1 };

	auto& anim = manager.AddComponent<ecs::FbxAnimComponent>(entity);
	anim.Play(*fbx.Resource, "Attack_A", true);

	reg.emplace<ecs::ColliderComponent>(entity,
		ecs::ColliderComponent::MakeBox({ 1,3,1 }));

	reg.emplace<ecs::RigidBodyComponent>(entity,
		ecs::RigidBodyComponent::MakeDynamic());

}

void CreateSound()
{
	auto& AudioManager = audio::AudioManager::Get();
	AudioManager.PlaySE("Assets/SE/TestSE.aud");
	AudioManager.PlayBGM("Assets/SE/TestBGM.aud");
}

// テスト用のカメラ作成
entt::entity CreateCamera()
{
	auto& registry = ecs::EntityManager::Get().GetRegistry();

	entt::entity entity = ecs::EntityManager::Get().CreateEntity();

	auto& tr = registry.emplace<ecs::Transform>(entity);

	tr.SetPosition(0.0f, 1.0f, -100.0f);

	auto& cam = registry.emplace<ecs::CameraComponent>(entity);
	cam.IsMainCamera = true;
	cam.Fov = 60.0f;
	cam.Near = 0.1f;  // 近すぎてクリップするのを防ぐため 0.01f から 0.1f に推奨変更
	cam.Far = 1000.0f;
	cam.SetAspectRatioFromWindow(sys::Window::Get());

	return entity;
}

// テスト用のライト作成
void CreateLight()
{
	auto& registry = ecs::EntityManager::Get().GetRegistry();
	entt::entity entity = ecs::EntityManager::Get().CreateEntity();

	// 座標系
	auto& tr = registry.emplace<ecs::Transform>(entity);

	// ライト
	auto& light = registry.emplace<ecs::DirectionalLightComponent>(entity);

}

// テスト用のfieldの作成
void CreateField()
{
	auto& manager = ecs::EntityManager::Get();
	auto& registry = ecs::EntityManager::Get().GetRegistry();

	// Resource取得
	auto res = graphics::PrimitiveResourceManager::Get().GetResource("Field");
	float scale = 10;

	auto entity = manager.CreateEntity();
	auto& tr = manager.AddComponent<ecs::Transform>(entity);
	tr.SetScale(scale);

	auto& fbx = manager.AddComponent<ecs::FbxComponent>(entity);
	fbx.Resource = res;
	fbx.CustomColor = { 1,0,0,1 };

	registry.emplace<ecs::ColliderComponent>(entity,
		ecs::ColliderComponent::MakeBox({ 50.f, 0.5f, 50.f })); // 幅100 × 高さ1 × 奥行100

	registry.emplace<ecs::RigidBodyComponent>(entity,
		ecs::RigidBodyComponent::MakeStatic());

}

void CreateDebugObject()
{
	// fbx
#if	DEBUG_FBX
	LoadResource();
	Create3DModel();
	CreateField();
#endif

	// sprite
#if	DEBUG_SPRITE
	SpriteRenderTest();
#endif

	// camera
#if	DEBUG_CAMERA
	CreateCamera();
#endif

	// sound
#if	DEBUG_SOUND
	CreateSound();
#endif

	// light
#if	DEBUG_LIGHT
	CreateLight();
#endif

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

		// ComponentSystemManagerの初期化
		mComponentSystemManager = &ecs::ComponentSystemManager::Get();
		mComponentSystemManager->ClearUserSystems();

		// 入力管理の初期化
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

		// PrimitiveModel
		SINGLETON_REF(graphics::PrimitiveResourceManager, PrimitiveResourceManager);
		PrimitiveResourceManager.Initialize();

		// カメラ
		if (sys::CameraSystem::Get().Initialize() == false)
		{
			return false;
		}

		// Audio
		SINGLETON_REF(audio::AudioManager, AudioManager);
		if (AudioManager.Initialize() == false)
		{
			return false;
		}
		SINGLETON_REF(audio::AudioDevice, AudioDevice);
		if(AudioDevice.Initialize(&AudioManager, 48000U) == false)
		{
			return false;
		}

		// 物理
		SINGLETON_REF(sys::PhysicsManager, PhysicsManager);
		if (PhysicsManager.Initialize(mEntityManager->GetRegistry()) == false)
		{
			return false;
		}

#ifdef _DEBUG
		if (graphics::PhysicsDebugRenderer::Get().Initialize() == false)
		{
			return false;
		}

#endif // _DEBUG

		// シーン
		mSceneManager = &sys::SceneManager::Get();
		mSceneManager->Initialize(sys::SceneFactory::Get().GetDefaultSceneName());


		// テスト用のインスタンス生成
		CreateDebugObject();

		InitializeDebugUI();

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

		// 更新
		this->Update();

		// 描画
		this->Render();

		// フレーム末の処理
		this->Conclude();

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

		// 物理
		sys::PhysicsManager::Get().Finalize();
#ifdef _DEBUG
		graphics::PhysicsDebugRenderer::Get().Finalize();
#endif // _DEBUG

		// TODO:ログ出力

		sys::Logger::Get().Finalize();

		return true;
	}

	void Engine::InitializeDebugUI()
	{
		auto& registry = mEntityManager->GetRegistry();

		// ライト
		sys::LightSystem::DebugUI(registry);

	}

	/// <summary>
	/// 状態更新
	/// </summary>
	void Engine::Update()
	{
		// 事前更新
		{
			// 時間経過
			mTime.Update();
		}

		// 取得
		auto& registry = ecs::EntityManager::Get().GetRegistry();
		auto dt = mTime.GetDeltaTime();
		float rawDt = mTime.GetRawDeltaTime();

		// メイン更新
		{
			
			mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::PreUpdate, registry, dt, rawDt);

			mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::Update, registry, dt, rawDt);

			sys::PhysicsSystem::BuildPendingBodies(registry);
			sys::PhysicsSystem::SyncFromTransform(registry);

			while (mTime.AccumulateFixedStep())
			{
				// 物理演算の更新 固定ステップにする。
				sys::PhysicsSystem::Update(registry, mTime.GetFixedDeltaTime());
			}
			sys::PhysicsSystem::SyncToTransform(registry);

			// 座標更新、行列更新

			// 衝突イベント発火

			mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::PostUpdate, registry, dt, rawDt);
		}

		// 事後更新
		{
			// FBXアニメーション時間の更新
			graphics::FbxAnimSystem::Update(registry, dt);

			// カメラ行列の更新
			sys::CameraSystem::Get().Update(registry);

			// ライトの更新
			sys::LightSystem::Update(registry);

		}
	}

	/// <summary>
	/// 描画
	/// </summary>
	void Engine::Render()
	{
		auto context = mDX12Renderer->GetContext();

		auto& registry = mEntityManager->GetRegistry();
		auto cmdList = context->GetCommandList();

		// Begin
		{
			mDX12Renderer->BeginFrame();
			graphics::RenderContext::Get().SetFrameIndex(context->GetCurrentFrameIndex());

			mImGuiManager->NewFrame();
			mImGuiManager->Update();
		}

		// Draw
		{
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

#ifdef _DEBUG

			graphics::PhysicsDebugRenderer::Get().Draw(registry, cmdList);
#endif // _DEBUG

		}

		// End
		{

			mImGuiManager->EndFrame();
			mDX12Renderer->EndFrame();

		}
	}

	/// <summary>
	/// フレーム末の処理
	/// </summary>
	void Engine::Conclude()
	{
		// フレーム末の処理
		mInputManager->Update();
	}
}
