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
#include<graphics/Line/Renderer/LightDebugRenderer.h>

// Editor (_DEBUG専用の配置/選択/Play-Stopワークフロー)
#include<system/Editor/EditorManager.h>
#include<system/Editor/EditorSystem.h>
#include<system/Editor/EditorUI.h>
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
#include<system/Time/TimeManager.h>
#include<system/Time/PerformanceMonitor.h>

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
#if !DEV_TOOL_ENABLED
        // Releaseビルドのみ、設定ファイルの値に関わらず強制的にフルスクリーンで起動する
        wctx.IsFullScreen = true;
#endif
        return EngineContext({ wctx });
    }

    bool Engine::Initialize()
    {
        if (sys::Logger::Get().Initialize() == false) return false;

        SINGLETON_REF(sys::AssetPathManager, AssetManager);
        sys::AssetPathManager::Get().Initialize();

        // App側の資産読込は "Assets/..." のような生の相対パス(CWD基準)に依存している。
        // VSデバッグ実行時はCWDが既定でApp直下になるため問題無いが、
        // ビルド後のexeを直接起動するとCWDがexe自身のフォルダ(x64/Release等)になり、
        // 直下にAssetsが無いため読込に失敗する。AssetPathManagerが探索済みのGameルート
        // (App直下)へCWDを固定し、起動方法に関わらず解決先を一致させる。
        const auto gameRoot = sys::AssetPathManager::Get().Resolve("/Game");
        if (!gameRoot.empty())
        {
            std::error_code ec;
            std::filesystem::current_path(gameRoot, ec);
        }

        mTimeManager = &::sys::TimeManager::Get();

        mTimeManager->Initialize();

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

        // 描画コマンド記録用ワーカーを停止する（以降 Render() は呼ばれない）
        if (mRenderThreadPool != nullptr)
            mRenderThreadPool->Finalize();

        FbxRenderer::Get().Finalize();
        SkyboxRenderer::Get().Finalize();
        PrimitiveResourceManager::Get().Finalize();
        EffekseerManager::Get().Finalize();
        TextRenderer::Get().Finalize();
        SpriteRenderer::Get().Finalize();
        ShapeRenderer::Get().Finalize();
        TransitionRenderer::Get().Finalize();

        sys::PerformanceMonitor::Get().Finalize();

#if DEV_TOOL_ENABLED
        graphics::PhysicsDebugRenderer::Get().Finalize();
        graphics::LightDebugRenderer::Get().Finalize();
        sys::EditorUI::Get().Finalize();
#endif

        // リソースマネージャーのクリア
        FbxResourceManager::Get().Clear();
        TextureManager::Get().Clear();

        // UIの終了
        ImGuiManager::Get().Finalize();

        // 物理エンジンなどの終了
        sys::PhysicsManager::Get().Finalize();

        // 基盤（ヒープマネージャ）の終了（すべてが解放された後に呼ぶ）
        graphics::GDescriptorHeapManager::Get().Finalize();

        // DX12 デバイス/レンダラーの破棄
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

        // 描画コマンド記録用ワーカープール（Shadow / Scene / Sprite の3チャネル分）
        mRenderThreadPool = std::make_unique<utility::ThreadPool>();
        mRenderThreadPool->Initialize(3);

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
#if DEV_TOOL_ENABLED
        if (graphics::PhysicsDebugRenderer::Get().Initialize() == false) return false;
        if (graphics::LightDebugRenderer::Get().Initialize() == false) return false;
        if (sys::EditorUI::Get().Initialize(mEntityManager->GetRegistry()) == false) return false;
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

        sys::PerformanceMonitor::Get().Initialize();
    }

    void Engine::Update()
    {
        auto& time = GetTime();
        time.Update();

        auto& registry = ecs::EntityManager::Get().GetRegistry();
        auto  dt = time.GetDeltaTime();
        float rawDt = time.GetRawDeltaTime();

        sys::PerformanceMonitor::Get().RecordFrame(rawDt);

#if DEV_TOOL_ENABLED
        // Editモード中はゲームロジック・物理・アニメ・エフェクトを一切動かさず、
        // 表示に必要な最小限のシステムとエディタ操作(選択/配置/ドラッグ)のみ実行する。
        // Playモードでは従来通りフル更新する。
        if (sys::EditorManager::Get().IsPlaying())
        {
            UpdateGameplay(dt, rawDt, registry);
        }
        else
        {
            sys::CameraSystem::Get().Update(registry);
            sys::LightSystem::Update(registry);
            sys::EditorSystem::Get().Update(registry);
        }
#else
        // Releaseビルドにはエディタ機能自体が無いため、常にフル更新する。
        UpdateGameplay(dt, rawDt, registry);
#endif
    }

    void Engine::UpdateGameplay(float dt, float rawDt, entt::registry& registry)
    {
        auto& time = GetTime();

        {
            sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::GameplayUpdate);
            mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::PreUpdate, registry, dt, rawDt);
            mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::Update, registry, dt, rawDt);
            sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::GameplayUpdate);

            sys::PhysicsSystem::ClearCollisionEvents(registry);

            mSceneManager->Update(rawDt);

            sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::Physics);
            sys::PhysicsSystem::BuildPendingBodies(registry);
            sys::PhysicsSystem::SyncFromTransform(registry);

            ::sys::PhysicsSystem::ApplyMoveVelocity(registry, time.GetFixedDeltaTime());

            while (time.AccumulateFixedStep())
                sys::PhysicsSystem::Update(registry, time.GetFixedDeltaTime());

            sys::PhysicsSystem::SyncToTransform(registry);
            sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::Physics);

            sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::GameplayUpdate);
            mComponentSystemManager->ExecutePhase(ecs::eUpdatePhase::PostUpdate, registry, dt, rawDt);
            sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::GameplayUpdate);
        }

        {
            graphics::FbxAnimSystem::Update(registry, dt);
            sys::CameraSystem::Get().Update(registry);

            // ライト更新 (LightViewProj の計算も含む)
            sys::LightSystem::Update(registry);

            sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::EffectUpdate);
            graphics::EffekseerManager::Get().Update(registry, dt);
            sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::EffectUpdate);
        }
    }

    /// <summary>
    /// 描画。
    /// フレームは以下の 2 フェーズ
    /// 収集フェーズ  Begin() / UpdateAndDraw()
    ///
    /// 記録フェーズ  End() / Flush()
    /// </summary>
    void Engine::Render()
    {
        using namespace graphics;

        auto  context = mDX12Renderer->GetContext();
        auto& registry = mEntityManager->GetRegistry();

        // begin
        {
            mDX12Renderer->BeginFrame();
            mImGuiManager->NewFrame();
            mImGuiManager->Update();
        }

        SINGLETON_REF(graphics::FbxRenderer, fbxRenderer);
        SINGLETON_REF(graphics::SkyboxRenderer, skyboxRenderer);
        SINGLETON_REF(graphics::SpriteRenderer, spriteRenderer);
        SINGLETON_REF(graphics::ShapeRenderer, shapeRenderer);
        SINGLETON_REF(graphics::TextRenderer, textRenderer);

        // 収集フェーズ
        {
            sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::RenderCollect);

            fbxRenderer.Begin();
            fbxRenderer.UpdateAndDraw(registry);

            skyboxRenderer.Begin();
            skyboxRenderer.UpdateAndDraw(registry);

            spriteRenderer.Begin();
            spriteRenderer.UpdateAndDraw(registry);

            shapeRenderer.Begin();
            shapeRenderer.UpdateAndDraw(registry);

            textRenderer.Begin();
            textRenderer.UpdateAndDraw(registry);

#if DEV_TOOL_ENABLED
            SINGLETON_REF(graphics::PhysicsDebugRenderer, physicsDebugRenderer);
            physicsDebugRenderer.Begin();
            physicsDebugRenderer.UpdateAndDraw(registry);

            SINGLETON_REF(graphics::LightDebugRenderer, lightDebugRenderer);
            lightDebugRenderer.Begin();
            lightDebugRenderer.UpdateAndDraw(registry);
#endif

            sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::RenderCollect);
        }

        // 記録フェーズ
        {
            std::function<void()> parallelTasks[3] =
            {
                // Shadow: RTV を外して Shadow Mapに深度を書き込む。
                [&]()
                {
                    sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::ShadowPass);
                    fbxRenderer.DrawShadowPass(
                        context->GetCommandList(eRenderChannel::Shadow));
                    sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::ShadowPass);
                },
                // Scene: 通常描画パスと Skybox
                [&]()
                {
                    sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::ScenePass);
                    auto* cmdList = context->GetCommandList(eRenderChannel::Scene);
                    fbxRenderer.End(cmdList);
                    skyboxRenderer.End(cmdList, fbxRenderer.GetSceneBufferGpuHandle());
                    sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::ScenePass);
                },
                // Sprite: 2D 描画（Sprite → Shape → Text の順）
                [&]()
                {
                    sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::SpritePass);
                    auto* cmdList = context->GetCommandList(eRenderChannel::Sprite);
                    spriteRenderer.End(cmdList);
                    shapeRenderer.End(cmdList);
                    textRenderer.Flush(cmdList);
                    sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::SpritePass);
                },
            };
            mRenderThreadPool->Dispatch(parallelTasks, 3);

            // Effect: Effekseer はスレッドセーフでないため専用チャネルに隔離し、
            //         ワーカーと並行してメインスレッドで記録する。
            sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::EffectDraw);
            EffekseerManager::Get().Draw(
                registry, context->GetCommandList(eRenderChannel::Effect));
            sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::EffectDraw);

            // Debug: デバッグ描画・トランジション・ImGui。
            //        いずれもスレッドセーフでないためメインスレッドで記録する。
            {
                sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::Debug);
                auto* cmdList = context->GetCommandList(eRenderChannel::Debug);

#if DEV_TOOL_ENABLED
                graphics::PhysicsDebugRenderer::Get().End(cmdList);
                graphics::LightDebugRenderer::Get().End(cmdList);
#endif
                // シーントランジションのフルスクリーンオーバーレイ
                mSceneManager->DrawTransition(cmdList);

                // ImGui はスレッドセーフでないため Debug チャネルで記録する。
                mImGuiManager->EndFrame(cmdList);
                sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::Debug);
            }

            mRenderThreadPool->WaitAll();
        }

        // End
        {
            mDX12Renderer->EndFrame();

            mDX12Renderer->WaitForGPU();
        }
    }

    void Engine::Conclude()
    {
        mInputManager->Update();
        mSceneManager->PostUpdate();
        ::sys::PhysicsSystem::ClearMoveVelocity(mEntityManager->GetRegistry());
    }

} // namespace sys