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
#include<graphics/Shape/Renderer/ShapeRenderer.h>
#include<graphics/Fbx/Renderer/FbxRenderer.h>
#include<graphics/Sprite/Renderer/SpriteRenderer.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>
#include<graphics/Transition/TransitionRenderer.h>

// Resource
#include<graphics/PrimitiveModel/Resource/PrimitiveResourceManager.h>
#include<graphics/Shader/ShaderManager.h>
#include<graphics/Texture/TextureManager.h>

// System
#include<graphics/Fbx/Animation/FbxAnimSystem.h>
#include<system/Camera/CameraSystem.h>
#include<system/Light/LightSystem.h>
#include<ecs/entity/EntityManager.h>
#include<ecs/system/manager/ComponentSystemManager.h>

// Physics
#include<system/Physics/System/PhysicsSystem.h>
#include<system/Physics/Manager/PhysicsManager.h>

// Scene
#include<system/Scene/Manager/SceneManager.h>
#include<system/Scene/Factory/SceneFactory.h>

// data
#include<Data/Storage/Registry/DataRegistry.h>

// Config
#include<Config/WindowConfig.h>

// Resoruce
#include<graphics/Fbx/Resource/FbxResourceManager.h>

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

        // オーディオ
        if (InitializeAudio() == false) return false;

        // 各種システム
        if (InitializeSystem() == false) return false;

        // デバック用UI
        this->InitializeDebugUI();

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
        SkyboxRenderer::Get().Finalize();
        PrimitiveResourceManager::Get().Finalize();
        EffekseerManager::Get().Finalize();
        TextRenderer::Get().Finalize();
        SpriteRenderer::Get().Finalize();
        ShapeRenderer::Get().Finalize();
        TransitionRenderer::Get().Finalize();

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

        if (ShapeRenderer::Get().Initialize(
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

        if (TransitionRenderer::Get().Initialize() == false)
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
        // データベース初期化
        ::data::DataRegistry::Get().Init("Assets/Bin/DB/db.db");

        // シーン生成
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
            mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::PreUpdate, registry, dt, rawDt);
            mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::Update, registry, dt, rawDt);

            // シーン切り替えリクエスト（ChangeSceneWithTransition 等）は上記の Update フェーズ内、
            // 例えば TitleInputSystem::Update で発行される。
            // そのため SceneManager::Update（フェード進行）は各システムの実行後に呼び、
            // 同フレーム中にリクエストされたトランジションを 1フレーム遅延なく開始できるようにする。
            mSceneManager->Update(rawDt);

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

            // Shape
            SINGLETON_REF(graphics::ShapeRenderer, ShapeRenderer);
            ShapeRenderer.Begin();
            ShapeRenderer.UpdateAndDraw(registry);
            ShapeRenderer.End(cmdList);

            // テキスト
            SINGLETON_REF(graphics::TextRenderer, TextRenderer);
            TextRenderer.Begin();
            TextRenderer.UpdateAndDraw(registry);
            TextRenderer.Flush(cmdList);

#ifdef _DEBUG
            graphics::PhysicsDebugRenderer::Get().Draw(registry, cmdList);
#endif

            // シーントランジション（フェードイン/アウト）のフルスクリーンオーバーレイ。
            // すべてのシーン描画コマンドの後、EndFrame() より前に発行する必要がある。
            mSceneManager->DrawTransition(cmdList);
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