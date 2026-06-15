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

// Renderer
#include<graphics/Skybox/Renderer/SkyboxRenderer.h>

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

// Config
#include<Config/WindowConfig.h>

// Component
#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/camera/CameraComponent.h>
#include<ecs/component/Fbx/FbxComponent.h>
#include<ecs/component/Fbx/FbxAnimComponent.h>
#include<ecs/component/Light/LightComponent.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>
#include<ecs/component/skybox/SkyboxComponent.h>

// Resoruce
#include<graphics/Texture/Texture.h>
#include<graphics/Fbx/Resource/FbxResourceManager.h>
#include<graphics/Fbx/Resource/FbxResource.h>

// define
#include"EngineDefine.h"


void SpriteRenderTest()
{
    auto texture = graphics::TextureManager::Get().GetOrLoad("Assets/Test/test.png");
    auto entity = ecs::EntityManager::Get().CreateEntity();
    auto& tr = ecs::EntityManager::Get().AddComponent<ecs::Transform>(entity);
    auto& sprite = ecs::EntityManager::Get().AddComponent<ecs::Sprite>(entity, texture);
}

void LoadResource()
{
    {
        auto& manager = graphics::FbxResourceManager::Get();
        auto res = manager.Load("Assets/Fbx/Faul.fbx.bin");
        bool ret = manager.LoadAnm("Assets/Fbx/Faul.fbx.bin", "Assets/Fbx/Animation/Attack_A.fbx.anm", "Attack_A");
        ret = manager.LoadAnm("Assets/Fbx/Faul.fbx.bin", "Assets/Fbx/Animation/Attack_B.fbx.anm", "Attack_B");
    }
    {
        auto& ResManager = audio::AudioResourceManager::Get();
        auto res = ResManager.GetResource("Assets/SE/TestSE.aud");
    }
}

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

    reg.emplace<ecs::ColliderComponent>(entity, ecs::ColliderComponent::MakeBox({ 1,3,1 }));
    reg.emplace<ecs::RigidBodyComponent>(entity, ecs::RigidBodyComponent::MakeDynamic());
}

void CreateSound()
{
    auto& AudioManager = audio::AudioManager::Get();
    AudioManager.PlaySE("Assets/SE/TestSE.aud");
    AudioManager.PlayBGM("Assets/SE/TestBGM.aud");
}

entt::entity CreateCamera()
{
    auto& registry = ecs::EntityManager::Get().GetRegistry();
    entt::entity entity = ecs::EntityManager::Get().CreateEntity();

    auto& tr = registry.emplace<ecs::Transform>(entity);
    tr.SetPosition(0.0f, 1.0f, -100.0f);

    auto& cam = registry.emplace<ecs::CameraComponent>(entity);
    cam.IsMainCamera = true;
    cam.Fov = 60.0f;
    cam.Near = 0.1f;
    cam.Far = 1000.0f;
    cam.SetAspectRatioFromWindow(sys::Window::Get());

    return entity;
}

void CreateLight()
{
    auto& registry = ecs::EntityManager::Get().GetRegistry();
    entt::entity entity = ecs::EntityManager::Get().CreateEntity();

    registry.emplace<ecs::Transform>(entity);

    auto& light = registry.emplace<ecs::DirectionalLightComponent>(entity);
    light.Direction = { 0.3f, -1.0f, 0.5f }; // 斜め下向き
    light.Color = { 1.0f,  1.0f, 1.0f };
    light.Intensity = 1.0f;
    light.IsActive = true;

    // ── Shadow 設定 ────────────────────────────────────────
    // CastShadow = true にするだけで Shadow Map が生成される。
    // ShadowRange / ShadowTarget / ShadowDistance はシーン規模に合わせて調整。
    light.CastShadow = true;
    light.ShadowRange = 50.0f;   // 50x50 ユニットのエリアをカバー
    light.ShadowTarget = { 0.0f, 0.0f, 0.0f }; // シーン中心
    light.ShadowDistance = 30.0f;   // ライト位置をターゲットから 30 ユニット離す
    light.ShadowNear = 0.1f;
    light.ShadowFar = 200.0f;
    light.ShadowBias = 0.005f;  // アクネが出たら増やす
}

void CreateField()
{
    auto& manager = ecs::EntityManager::Get();
    auto& registry = manager.GetRegistry();

    auto res = graphics::PrimitiveResourceManager::Get().GetResource("Field");
    float scale = 10;

    auto entity = manager.CreateEntity();
    auto& tr = manager.AddComponent<ecs::Transform>(entity);
    tr.SetScale(scale);

    auto& fbx = manager.AddComponent<ecs::FbxComponent>(entity);
    fbx.Resource = res;
    fbx.CustomColor = { 1,0,0,1 };

    registry.emplace<ecs::ColliderComponent>(entity,
        ecs::ColliderComponent::MakeBox({ 50.f, 0.5f, 50.f }));
    registry.emplace<ecs::RigidBodyComponent>(entity,
        ecs::RigidBodyComponent::MakeStatic());
}

void CreateSkybox()
{
    auto& manager = ecs::EntityManager::Get();
    auto entity = manager.CreateEntity();
    auto& skybox = manager.AddComponent<ecs::SkyboxComponent>(entity);
    skybox.TexturePath = "Assets/Skybox/skybox.dds";
}

void CreateDebugObject()
{
#if DEBUG_FBX
    LoadResource();
    Create3DModel();
    CreateField();
#endif
#if DEBUG_SPRITE
    SpriteRenderTest();
#endif
#if DEBUG_CAMERA
    CreateCamera();
#endif
#if DEBUG_SOUND
    CreateSound();
#endif
#if DEBUG_LIGHT
    CreateLight();
#endif
    CreateSkybox();
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

    EngineContext Engine::LoadBootstrapConfig()
    {
        data::WindowConfig WindowCfg = LoadWindowConfig(ASSET_PATH("/Engine/BootstrapConfig/WindowConfig.json"));
        WindowContext wctx;
        wctx.VirtualWidth = 1920;
        wctx.VirtualHeight = 1080;
        data::ApplyWindowConfig(WindowCfg, wctx);
        return EngineContext({ wctx });
    }

    bool Engine::Initialize()
    {
        if (sys::Logger::Get().Initialize() == false) return false;

        SINGLETON_REF(sys::AssetPathManager, AssetManager);
        AssetManager.Initialize();

        auto context = LoadBootstrapConfig();
        mTime.Initialize();

        mWindow = &Window::Get();
        if (mWindow->Initialize(context.WindowContext) == false) return false;

        mDevice = &graphics::DX12Device::Get();
        if (mDevice->Initialize() == false) return false;

        mDX12Renderer = &graphics::DX12Renderer::Get();
        if (mDX12Renderer->Initialize(
            mDevice,
            mWindow->GetHWND(),
            mWindow->GetWidth(),
            mWindow->GetHeight()) == false) return false;

        auto& descriptorHeapManager = graphics::GDescriptorHeapManager::Get();
        if (descriptorHeapManager.Initialize(mDevice->GetDevice()) == false) return false;

        mImGuiManager = &sys::ImGuiManager::Get();
        if (mImGuiManager->Initialize(
            *mWindow, *mDevice,
            *mDX12Renderer->GetContext(),
            descriptorHeapManager) == false) return false;

        mEntityManager = &ecs::EntityManager::Get();
        if (mEntityManager->Initialize() == false) return false;

        mComponentSystemManager = &ecs::ComponentSystemManager::Get();
        mComponentSystemManager->ClearUserSystems();

        mInputManager = &sys::InputManager::Get();
        if (mInputManager->Initialize() == false) return false;

        InitializeRenderer(descriptorHeapManager);

        if (sys::CameraSystem::Get().Initialize() == false) return false;

        SINGLETON_REF(audio::AudioManager, AudioManager);
        if (AudioManager.Initialize() == false) return false;
        SINGLETON_REF(audio::AudioDevice, AudioDevice);
        if (AudioDevice.Initialize(&AudioManager, 48000U) == false) return false;

        SINGLETON_REF(sys::PhysicsManager, PhysicsManager);
        if (PhysicsManager.Initialize(mEntityManager->GetRegistry()) == false) return false;

#ifdef _DEBUG
        if (graphics::PhysicsDebugRenderer::Get().Initialize() == false) return false;
#endif

        mSceneManager = &sys::SceneManager::Get();
        mSceneManager->Initialize(sys::SceneFactory::Get().GetDefaultSceneName());

        CreateDebugObject();
        InitializeDebugUI();

        mIsRunning = true;
        mIsInitialized = true;

        DEBUG_LOG(sys::eLogLevel::Log, "Engine initialized successfully.");
        return true;
    }

    bool Engine::Run()
    {
        if (mIsInitialized == false) return false;

        mWindow->ProcessMessages();
        if (mWindow->IsQuitRequested())
        {
            mIsRunning = false;
            return false;
        }

        this->Update();
        this->Render();
        this->Conclude();
        return true;
    }

    bool Engine::Finalize()
    {
        if (mIsInitialized == false) return false;

        if (mDX12Renderer != nullptr)
            mDX12Renderer->WaitForGPU();

        mDX12Renderer->Finalize();
        mDX12Renderer = nullptr;

        mDevice->Finalize();
        mDevice = nullptr;

        sys::PhysicsManager::Get().Finalize();
#ifdef _DEBUG
        graphics::PhysicsDebugRenderer::Get().Finalize();
#endif

        mSceneManager->PostUpdate();
        sys::Logger::Get().Finalize();
        return true;
    }

    bool Engine::InitializeRenderer(graphics::GDescriptorHeapManager& descriptorHeapManager)
    {
        using namespace graphics;

        if (SpriteRenderer::Get().Initialize(
            *mDevice, descriptorHeapManager,
            graphics::ShaderManager::Get(), *mWindow) == false) return false;

        if (FbxRenderer::Get().Initialize(
            *mDevice, descriptorHeapManager,
            graphics::ShaderManager::Get()) == false) return false;

        PrimitiveResourceManager::Get().Initialize();

        if (SkyboxRenderer::Get().Initialize() == false) return false;

        return true;
    }

    void Engine::InitializeDebugUI()
    {
        auto& registry = mEntityManager->GetRegistry();
        sys::LightSystem::DebugUI(registry);
    }

    void Engine::Update()
    {
        { mTime.Update(); }

        auto& registry = ecs::EntityManager::Get().GetRegistry();
        auto  dt = mTime.GetDeltaTime();
        float rawDt = mTime.GetRawDeltaTime();

        {
            mSceneManager->Update(rawDt);

            mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::PreUpdate, registry, dt, rawDt);
            mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::Update, registry, dt, rawDt);

            sys::PhysicsSystem::BuildPendingBodies(registry);
            sys::PhysicsSystem::SyncFromTransform(registry);

            while (mTime.AccumulateFixedStep())
                sys::PhysicsSystem::Update(registry, mTime.GetFixedDeltaTime());

            sys::PhysicsSystem::SyncToTransform(registry);

            mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::PostUpdate, registry, dt, rawDt);
        }

        {
            graphics::FbxAnimSystem::Update(registry, dt);
            sys::CameraSystem::Get().Update(registry);

            // ライト更新 (LightViewProj の計算も含む)
            sys::LightSystem::Update(registry);
        }
    }

    void Engine::Render()
    {
        auto  context = mDX12Renderer->GetContext();
        auto& registry = mEntityManager->GetRegistry();
        auto  cmdList = context->GetCommandList();

        // ── Begin ─────────────────────────────────────────────
        {
            mDX12Renderer->BeginFrame();
            graphics::RenderContext::Get().SetFrameIndex(context->GetCurrentFrameIndex());
            mImGuiManager->NewFrame();
            mImGuiManager->Update();
        }

        // ── Draw ──────────────────────────────────────────────
        {
            SINGLETON_REF(graphics::FbxRenderer, FbxRenderer);

            // 1. フレームデータを収集
            FbxRenderer.Begin();
            FbxRenderer.UpdateAndDraw(registry);

            // 2. Shadow Pass
            //    RTV を外して Shadow Map (DSV) に深度を書き込む。
            //    BeginFrame でセットした RTV が上書きされるので、
            //    直後に RestoreMainRenderTarget で元に戻す。
            FbxRenderer.DrawShadowPass(cmdList);

            // 3. メインの RTV / DSV / ビューポートを再セット
            context->RestoreMainRenderTarget(cmdList);

            // 4. 通常描画パス (Shadow Map は SRV として t10 にバインド済み)
            FbxRenderer.End(cmdList);

            // skybox
            SINGLETON_REF(graphics::SkyboxRenderer, SkyboxRenderer);
            SkyboxRenderer.Begin();
            SkyboxRenderer.UpdateAndDraw(registry);
            SkyboxRenderer.End(cmdList, FbxRenderer.GetSceneBufferGpuHandle());

            // 2D Sprite
            SINGLETON_REF(graphics::SpriteRenderer, spriteRenderer);
            spriteRenderer.Begin();
            spriteRenderer.UpdateAndDraw(registry);
            spriteRenderer.End(cmdList);

#ifdef _DEBUG
            graphics::PhysicsDebugRenderer::Get().Draw(registry, cmdList);
#endif
        }

        // ── End ───────────────────────────────────────────────
        {
            mImGuiManager->EndFrame();
            mDX12Renderer->EndFrame();
        }
    }

    void Engine::Conclude()
    {
        mInputManager->Update();
        mSceneManager->PostUpdate();
    }

} // namespace sys