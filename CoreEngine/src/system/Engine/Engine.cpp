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
#include<graphics/Text/Renderer/TextRenderer.h>
#include<graphics/Effect/Manager/EffectManager.h>

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
#include<ecs/component/Text/TextComponent.h>
#include<ecs/component/Effect/EffectComponent.h>

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

void CreateText()
{
    auto& manager = ecs::EntityManager::Get();
    auto entity = manager.CreateEntity();
    auto& text = manager.AddComponent<ecs::TextComponent>(entity);
    text.Text = L"日本語テスト";
    text.Size = 64;
    text.Layer  = 0;
    text.X = 200;
    text.Y = 200;
}

void CreateSkybox()
{
    auto& manager = ecs::EntityManager::Get();
    auto entity = manager.CreateEntity();
    auto& skybox = manager.AddComponent<ecs::SkyboxComponent>(entity);
    skybox.TexturePath = "Assets/Skybox/skybox.dds";
}

void CreateEffect()
{
    auto& manager = ecs::EntityManager::Get();
    auto entity = manager.CreateEntity();
    auto& transform = manager.AddComponent<ecs::Transform>(entity);
    transform.SetScale(10);
    transform.SetPosition(100, 100, 0);

    auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
    effect.Asset = graphics::EffekseerManager::Get().GetEffect("Assets/Effect/Light3.efk");
    effect.IsLoop = true;
    effect.Effect.Play(effect.Asset, effect.Offset);
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
#if DEBUG_TEXT
    CreateText();
#endif
#if DEBUG_EFFECT
    CreateEffect();
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
        sys::AssetPathManager::Get().Initialize();
        mTime.Initialize();

        // コア部分の初期化（Window Dx12など）
        if (this->InitializeCore() == false) return false;
        auto& descriptorHeapManager = graphics::GDescriptorHeapManager::Get();

        // 各種レンダラー
        this->InitializeRenderer(descriptorHeapManager);

        // 各種システム
        if (InitializeSystem() == false) return false;

        // オーディオ
        if (InitializeAudio() == false) return false;

        // デバック用UI
        this->InitializeDebugUI();

        CreateDebugObject();

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
        using namespace graphics;

        if (mIsInitialized == false) return false;

        if (mDX12Renderer != nullptr)
            mDX12Renderer->WaitForGPU();

        FbxRenderer::Get().Finalize();
        SkyboxRenderer::Get().Finalize(); // ★ここに追加
        PrimitiveResourceManager::Get().Finalize();
        EffekseerManager::Get().Finalize();
        TextRenderer::Get().Finalize();
        SpriteRenderer::Get().Finalize();

#ifdef _DEBUG
        graphics::PhysicsDebugRenderer::Get().Finalize();
#endif

        // 2. リソースマネージャーのクリア（ここで Texture 等のディスクリプタが解放される）
        FbxResourceManager::Get().Clear();
        TextureManager::Get().Clear();

        // 3. UIの終了
        ImGuiManager::Get().Finalize();

        // 4. 物理エンジンなどの終了
        sys::PhysicsManager::Get().Finalize();

        // 5. 基盤（ヒープマネージャ）の終了（すべてが解放された後に呼ぶ）
        graphics::GDescriptorHeapManager::Get().Finalize();

        // 6. DX12 デバイス/レンダラーの破棄
        mDX12Renderer->Finalize();
        mDX12Renderer = nullptr;

        mDevice->Finalize();
        mDevice = nullptr;

        sys::Logger::Get().Finalize();
        return true;
    }

    /// <summary>
    /// コア部分の初期化
    /// </summary>
    /// <returns></returns>
    bool Engine::InitializeCore()
    {
        // 初期化用データ
        auto context = LoadBootstrapConfig();

        // ウィンドウ
        mWindow = &Window::Get();
        if (mWindow->Initialize(context.WindowContext) == false) return false;

        // デバイス
        mDevice = &graphics::DX12Device::Get();
        if (mDevice->Initialize() == false) return false;

        // レンダラー
        mDX12Renderer = &graphics::DX12Renderer::Get();
        if (mDX12Renderer->Initialize(
            mDevice,
            mWindow->GetHWND(),
            mWindow->GetWidth(),
            mWindow->GetHeight()) == false) return false;

        // ディスクリプタヒープ
        auto& descriptorHeapManager = graphics::GDescriptorHeapManager::Get();
        if (descriptorHeapManager.Initialize(mDevice->GetDevice()) == false) return false;

        // ImGui
        mImGuiManager = &sys::ImGuiManager::Get();
        if (mImGuiManager->Initialize(
            *mWindow, *mDevice,
            *mDX12Renderer->GetContext(),
            descriptorHeapManager) == false) return false;

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

        if (TextRenderer::Get().Initialize(
            *mDevice, descriptorHeapManager, graphics::ShaderManager::Get()) == false)
        {
            return false;
        }

        if (EffekseerManager::Get().Initialize(*mDevice, *graphics::DX12Renderer::Get().GetContext()) == false)
        {
            return false;
        }

        return true;
    }

    /// <summary>
    /// 各種システムの初期化
    /// </summary>
    /// <returns></returns>
    bool Engine::InitializeSystem()
    {
        // 入力
        mInputManager = &sys::InputManager::Get();
        if (mInputManager->Initialize() == false) return false;

        // エンティティ
        mEntityManager = &ecs::EntityManager::Get();
        if (mEntityManager->Initialize() == false) return false;

        // コンポーネント
        mComponentSystemManager = &ecs::ComponentSystemManager::Get();
        mComponentSystemManager->ClearUserSystems();

        // カメラ
        if (sys::CameraSystem::Get().Initialize() == false) return false;

        // 物理
        SINGLETON_REF(sys::PhysicsManager, PhysicsManager);
        if (PhysicsManager.Initialize(mEntityManager->GetRegistry()) == false) return false;
#ifdef _DEBUG
        if (graphics::PhysicsDebugRenderer::Get().Initialize() == false) return false;
#endif

        // シーン
        mSceneManager = &sys::SceneManager::Get();
        mSceneManager->Initialize(sys::SceneFactory::Get().GetDefaultSceneName());

        return true;
    }

    /// <summary>
    /// 音関係の初期化
    /// </summary>
    /// <returns></returns>
    bool Engine::InitializeAudio()
    {
        SINGLETON_REF(audio::AudioManager, AudioManager);
        if (AudioManager.Initialize() == false) return false;
        if (audio::AudioDevice::Get().Initialize(&AudioManager, 48000U) == false) return false;

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

            graphics::EffekseerManager::Get().Update(registry, dt);
        }
    }

    void Engine::Render()
    {
        auto  context = mDX12Renderer->GetContext();
        auto& registry = mEntityManager->GetRegistry();
        auto  cmdList = context->GetCommandList();

        // Begin
        {
            mDX12Renderer->BeginFrame();
            graphics::RenderContext::Get().SetFrameIndex(context->GetCurrentFrameIndex());
            mImGuiManager->NewFrame();
            mImGuiManager->Update();
        }

        // Draw
        {
            SINGLETON_REF(graphics::FbxRenderer, FbxRenderer);

            // フレームデータ取得
            FbxRenderer.Begin();
            FbxRenderer.UpdateAndDraw(registry);

            //    RTV を外して Shadow Map (DSV) に深度を書き込む。
            FbxRenderer.DrawShadowPass(cmdList);

            // メインのRTV/DSV ビューポートを再セット
            context->RestoreMainRenderTarget(cmdList);

            // 通常描画パス (Shadow Map は SRV として t10 にバインド済み)
            FbxRenderer.End(cmdList);


            // skybox
            SINGLETON_REF(graphics::SkyboxRenderer, SkyboxRenderer);
            SkyboxRenderer.Begin();
            SkyboxRenderer.UpdateAndDraw(registry);
            SkyboxRenderer.End(cmdList, FbxRenderer.GetSceneBufferGpuHandle());

            // effect
            graphics::EffekseerManager::Get().Draw(registry, cmdList);

            // 2D Sprite
            SINGLETON_REF(graphics::SpriteRenderer, spriteRenderer);
            spriteRenderer.Begin();
            spriteRenderer.UpdateAndDraw(registry);
            spriteRenderer.End(cmdList);

            // テキスト
            SINGLETON_REF(graphics::TextRenderer, TextRenderer);
            TextRenderer.Begin();
            TextRenderer.UpdateAndDraw(registry);
            TextRenderer.Flush(cmdList);

#ifdef _DEBUG
            graphics::PhysicsDebugRenderer::Get().Draw(registry, cmdList);
#endif
        }

        // End
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