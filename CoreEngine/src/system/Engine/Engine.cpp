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
        return EngineContext({ wctx });
    }

    bool Engine::Initialize()
    {
        if (sys::Logger::Get().Initialize() == false) return false;

        SINGLETON_REF(sys::AssetPathManager, AssetManager);
        sys::AssetPathManager::Get().Initialize();

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
        // (以前は#ifdef _DEBUGで分岐していたため、DevelopビルドではEditorUI自体は
        // 初期化されるのにこの判定だけ効かず、Playを押していなくても常にフル更新される
        // 不整合になっていた。DEV_TOOL_ENABLEDは_DEBUGを畳み込み済みのため、
        // 判定は必ずこちらを使うこと(DebugConfig.h参照))
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

            // Update フェーズの各システムが CollisionEnterEvent/SensorEnterEvent を読み終えた直後に
            // クリアする。これらのイベントは前フレームの物理ステップ(下の PhysicsSystem::Update)が
            // 生成したものを Update フェーズで消費する設計(1フレーム遅延)のため、
            // このタイミングを逃す(=消費前にクリアする/クリアし忘れる)と、
            // 消費し損ねる、または同じ接触が毎フレーム蓄積し続けて過剰にダメージ判定されるバグになる。
            sys::PhysicsSystem::ClearCollisionEvents(registry);

            // シーン切り替えリクエスト（ChangeSceneWithTransition 等）は上記の Update フェーズ内、
            // 例えば TitleInputSystem::Update で発行される。
            // そのため SceneManager::Update（フェード進行）は各システムの実行後に呼び、
            // 同フレーム中にリクエストされたトランジションを 1フレーム遅延なく開始できるようにする。
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
    ///
    /// フレームは以下の 2 フェーズに厳密に分かれる。
    ///
    /// [収集フェーズ]  Begin() / UpdateAndDraw()
    ///   - entt::registry を読む
    ///   - GPU バッファへの Update()（StructuredBuffer 等）を行う
    ///   - テクスチャの遅延ロード（GDescriptorHeapManager::Issuance）を行う
    ///   → registry / ヒープマネージャに触れてよいのはこのフェーズのみ
    ///
    /// [記録フェーズ]  End() / Flush()
    ///   - コマンドリストへの記録のみを行う
    ///   - registry には一切触れない
    ///   - GPU バッファの Update() も行わない（GetGpuHandle() の読み取りのみ）
    ///   → チャネルごとにコマンドリストが分かれているため、
    ///      Shadow / Scene / Sprite はワーカースレッドで並列に記録する。
    ///      Effect（Effekseer）/ Debug（ImGui）はスレッドセーフでないため
    ///      メインスレッドで記録し、ワーカーと並行して実行する。
    ///
    /// チャネルの投入順（= 描画順）は eRenderChannel の宣言順で保証される
    /// （記録順が並列化で入れ替わっても Flip() の ExecuteCommandLists 順は変わらない）。
    /// </summary>
    void Engine::Render()
    {
        using namespace graphics;

        auto  context = mDX12Renderer->GetContext();
        auto& registry = mEntityManager->GetRegistry();

        // ── Begin ────────────────────────────────────────────────
        // BeginRendering() 内で RenderContext::SetFrameIndex() が呼ばれ、
        // 全チャネルのコマンドリストが開かれる。
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

        // ── 収集フェーズ ──────────────────────────────────────────
        // registry の読み取りと GPU バッファへの転送はすべてここで完結させる。
        {
            sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::RenderCollect);

            fbxRenderer.Begin();
            fbxRenderer.UpdateAndDraw(registry);

            // SkyboxRenderer は内部で TextureManager::GetOrLoad()（遅延ロード）を行う。
            // Issuance() はスレッドセーフではないため、必ずこのフェーズで呼ぶこと。
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

        // ── 記録フェーズ ──────────────────────────────────────────
        // 各チャネルへコマンドを記録する。registry には触れない。
        //
        // Shadow / Scene / Sprite はワーカースレッドへ委譲し、
        // その間メインスレッドで Effect / Debug チャネルを記録することで
        // 実際に並列に記録する。GPU への投入(ExecuteCommandLists)は
        // WaitAll() で全ワーカーの記録完了を待った後、Flip() がチャネルの
        // 宣言順に行うため、記録順が入れ替わっても描画順は保証される。
        {
            std::function<void()> parallelTasks[3] =
            {
                // Shadow: RTV を外して Shadow Map (DSV) に深度を書き込む。
                //         専用チャネルなので他チャネルの RTV 設定には影響しない
                //         （RestoreMainRenderTarget() は不要）。
                [&]()
                {
                    sys::PerformanceMonitor::Get().BeginSection(sys::ePerfSection::ShadowPass);
                    fbxRenderer.DrawShadowPass(
                        context->GetCommandList(eRenderChannel::Shadow));
                    sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::ShadowPass);
                },
                // Scene: 通常描画パス（Shadow Map は SRV としてバインド済み）と Skybox
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
                // シーントランジション（フェードイン/アウト）のフルスクリーンオーバーレイ。
                // すべてのシーン描画コマンドの後に発行する必要がある。
                mSceneManager->DrawTransition(cmdList);

                // ImGui はスレッドセーフでないため Debug チャネル（メインスレッド）で記録する。
                mImGuiManager->EndFrame(cmdList);
                sys::PerformanceMonitor::Get().EndSection(sys::ePerfSection::Debug);
            }

            // Shadow / Scene / Sprite チャネルの記録完了を待つ。
            // parallelTasks はローカル変数のため、EndFrame() で Close するより前に
            // 必ず全ワーカーの記録が終わっていなければならない。
            mRenderThreadPool->WaitAll();
        }

        // ── End ──────────────────────────────────────────────────
        // 全チャネルを Close し、宣言順に ExecuteCommandLists → Present。
        {
            mDX12Renderer->EndFrame();

            // 【2026-07-21】EffekseerRendererLLGIの頂点リングバッファ(VertexBuffer::Unlock/
            // GetNextBuffer)にはGPU側の完了確認が無く、"nextIndex_"という単純なカウンタを
            // 描画バッチ(Unlock呼び出し)ごとに回しているだけ(ベンダーコード自身に
            // "TODO make correct ring buffer"というコメントあり、未完成の実装)。
            // このカウンタは「フレーム単位」ではなく「バッチ単位」で進むため、エンジン側で
            // フレーム単位のフェンス追跡(EffekseerManager::OnFrameSubmitted、現在は不使用)を
            // 行ってもバッチ数がフレームごとに変動する限り正しく保護できない
            // (試したが再発を確認済み)。CPUがGPUより先行しすぎる環境(Develop/Releaseの高fps)では
            // GPUがまだ読んでいるリング領域をCPUが上書きし、パーティクルがノイズ状に崩れる。
            // 正しく直すにはベンダーコード側にバッチ単位のフェンス管理を追加する必要があり
            // リスクが大きいため、確実性を優先してここで毎フレーム完全同期する。
            // パフォーマンスコストとの兼ね合いは要測定(PerformanceMonitor参照)。
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