#include "pch.h"
#include"EffectManager.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Dx12/Dx12Context.h>
#include <ecs/component/effect/EffectComponent.h>
#include <ecs/component/transform/TransformComponent.h>
#include<system/Camera/CameraSystem.h>
#include<ecs/component/camera/CameraComponent.h>

#include<algorithm>
#include<thread>

namespace
{
    /// <summary>
    /// EffekseerへのSetLocation/SetScale/SetRotation呼び出しを間引くための一致判定。
    /// 比較対象は毎フレーム同一の入力から算出される値のため、変化がなければビット単位で
    /// 一致する。許容誤差を設けると微小な移動が反映されなくなるため厳密比較にする。
    /// </summary>
    bool IsSameFloat3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
}

namespace graphics
{
    // -----------------------------------------------------------------------
    //  Initialize
    //  旧実装の EffectManager::Initialize() を参考に
    //  EffekseerRendererDX12::Create() でレンダラーを生成する
    // -----------------------------------------------------------------------
    bool EffekseerManager::Initialize(
        graphics::DX12Device& device,
        graphics::DX12Context& context)
    {
        if (mIsInitialized) return false;

        // RTV フォーマット（スワップチェーンと一致させる）
        DXGI_FORMAT rtFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

        mRenderer = EffekseerRendererDX12::Create(
            device.GetDevice(),
            context.GetCommandQueue(),
            graphics::FRAME_COUNT,
            &rtFormat,
            1,
            DXGI_FORMAT_D32_FLOAT,
            false,
            MAX_SQUARES);

        if (mRenderer == nullptr)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "EffekseerManager: Failed to create Renderer.");
            return false;
        }

        mManager = Effekseer::Manager::Create(MAX_INSTANCES);
        if (mManager == nullptr)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "EffekseerManager: Failed to create Manager.");
            return false;
        }

        // パーティクル更新(Manager::Update)をワーカースレッドへ分散する。
        //
        // 過去に一度有効化して表示崩れ(素の四角形で描画される/一度も表示されない)が出たが、
        // 原因は非同期化そのものではなく、非表示解除をIsPlaying()のタイミングに依存させて
        // いた当時の実装にあった。現在はフレーム数で管理しており環境非依存になっている
        // (ecs::EffectComponent::HiddenFramesRemaining参照)。
        //
        // Update()はSyncUpdate=true(既定)のため、ワーカーでの計算完了を待って返る。
        // 呼び出し側から見た同期点は単一スレッド時と同一で、余分な遅延やフレーム跨ぎの
        // 状態ずれは発生しない。並列化はDoUpdate内のインスタンス分割に対して効く。
        if constexpr (EFFECT_WORKER_THREAD_COUNT > 0)
        {
            mManager->LaunchWorkerThreads(EFFECT_WORKER_THREAD_COUNT);
            DEBUG_LOG(sys::eLogLevel::Log,
                "EffekseerManager: Launched {} worker threads for particle update.",
                EFFECT_WORKER_THREAD_COUNT);
        }

        // レンダラーの設定
        mManager->SetSpriteRenderer(mRenderer->CreateSpriteRenderer());
        mManager->SetRibbonRenderer(mRenderer->CreateRibbonRenderer());
        mManager->SetRingRenderer(mRenderer->CreateRingRenderer());
        mManager->SetModelRenderer(mRenderer->CreateModelRenderer());
        mManager->SetTrackRenderer(mRenderer->CreateTrackRenderer());

        // ローダーの設定
        mManager->SetTextureLoader(mRenderer->CreateTextureLoader());
        mManager->SetModelLoader(mRenderer->CreateModelLoader());
        mManager->SetMaterialLoader(mRenderer->CreateMaterialLoader());

        // メモリプール・コマンドリスト
        mMemoryPool = EffekseerRenderer::CreateSingleFrameMemoryPool(
            mRenderer->GetGraphicsDevice());
        if (mMemoryPool == nullptr)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "EffekseerManager: Failed to create SingleFrameMemoryPool.");
            return false;
        }

        mCmdList = EffekseerRenderer::CreateCommandList(
            mRenderer->GetGraphicsDevice(), mMemoryPool);

        mIsInitialized = true;
        DEBUG_LOG(sys::eLogLevel::Log, "EffekseerManager: Initialized successfully.");
        return true;
    }

    void EffekseerManager::Finalize()
    {
        if (!mIsInitialized) return;

        StopAll();
        mEffectCache.clear();

        mCmdList.Reset();
        mMemoryPool.Reset();
        mManager.Reset();
        mRenderer.Reset();

        mIsInitialized = false;
        DEBUG_LOG(sys::eLogLevel::Log, "EffekseerManager: Finalized.");
    }

    // -----------------------------------------------------------------------
    //  Update  ― 旧 EffectSystem::PostUpdate() を参考に
    // -----------------------------------------------------------------------
    void EffekseerManager::Update(entt::registry& registry, float dt)
    {
        if (!mIsInitialized) return;

        // 素材別の負荷内訳は「どの.efkを削れば効くか」の判断材料。集計自体もコストのため
        // デバッグUIが存在するビルドでのみ行う(PerformanceMonitor::Initializeと同じ条件)
#if DEV_TOOL_ENABLED
        mEffectStats.clear();
        constexpr bool kCollectEffectStats = true;
#else
        constexpr bool kCollectEffectStats = false;
#endif

        registry.view<ecs::EffectComponent>().each([&](entt::entity entity, ecs::EffectComponent& effect)
            {
                if constexpr (kCollectEffectStats)
                {
                    // 非表示・再生終了の分岐で早期returnする前に集計する
                    // (GetTotalInstanceCount()の値と内訳が一致するようにするため)
                    AccumulateEffectStat(effect);
                }

                // 表示状態の変更を反映
                if (effect.IsVisible != effect.LastIsVisible)
                {
                    effect.Effect.SetVisible(effect.IsVisible);
                    effect.LastIsVisible = effect.IsVisible;
                }

                if (!effect.IsVisible) return;

                // Transform に追従する位置を解決する（無ければ Offset を直接座標として使う）。
                // Play() での初回再生位置と SetLocation() での追従先を同じ値にするため、
                // ここで一度だけ計算して使い回す（バラバラに計算すると再生直後の1フレームだけ
                // Offset(原点扱い)に表示されてから追従先へ飛ぶ、という表示不具合になる）。
                ecs::Transform* targetTrans = nullptr;

                if (effect.Parent != entt::null && registry.valid(effect.Parent))
                    targetTrans = registry.try_get<ecs::Transform>(effect.Parent);
                else
                    targetTrans = registry.try_get<ecs::Transform>(entity);

                const DirectX::XMFLOAT3 worldPos = targetTrans
                    ? DirectX::XMFLOAT3{
                        targetTrans->GetPosition().x + effect.Offset.x,
                        targetTrans->GetPosition().y + effect.Offset.y,
                        targetTrans->GetPosition().z + effect.Offset.z }
                    : effect.Offset;

                // 再生開始直後の非表示期間を進める。
                // IsPlaying()の状態に依存せずフレーム数だけで判定するため、
                // ワーカースレッドの有無で内部状態の確定タイミングが変わっても破綻しない
                if (effect.HiddenFramesRemaining > 0)
                {
                    --effect.HiddenFramesRemaining;
                    if (effect.HiddenFramesRemaining == 0)
                    {
                        effect.Effect.SetRenderingVisible(true);
                    }
                }

                // 再生終了していたら
                if (!effect.Effect.IsPlaying())
                {
                    if (effect.IsLoop == true && effect.Asset != nullptr)
                    {
                        // ループ: 現在の追従先座標から再スタートする。
                        // 再始動直後も新規生成と同じく内部状態が未確定のため、同じ猶予を与える
                        // (FrostOrbの周回オーブ・IceSpike・各種投射武器のトレイル等、
                        // IsLoop=trueで素材自体の長さより長く表示し続けたい場合に発生する)。
                        effect.Effect.Play(effect.Asset, worldPos, effect.Effect.ShouldDestroy());
                        MarkSpawnHidden(effect);
                        // Play()でEffekseer側の変換行列がリセットされるため、
                        // 差分チェックを無効化して下の適用処理で必ず再適用させる
                        effect.HasAppliedTransform = false;
                    }
                    else if (effect.HiddenFramesRemaining > 0)
                    {
                        // 生成直後の猶予中はIsPlaying()がまだfalseを返しうる。
                        // ここで破棄すると「一度も表示されずに消えるエフェクト」になるため、
                        // 猶予が明けるまでは破棄しない
                    }
                    else
                    {
                        // 自動削除フラグがあればエンティティを削除
                        if (effect.Effect.ShouldDestroy())
                            registry.destroy(entity);
                        return;
                    }
                }

                // スケール = Transform.Scale * EffectComponent.Scale
                const DirectX::XMFLOAT3 worldScale = targetTrans
                    ? DirectX::XMFLOAT3{
                        targetTrans->GetScale().x * effect.Scale.x,
                        targetTrans->GetScale().y * effect.Scale.y,
                        targetTrans->GetScale().z * effect.Scale.z }
                    : effect.Scale;

                // Effekseer側の各Setterはstd::map検索と行列再構築を伴うため、
                // 前回適用値から変化したものだけを呼ぶ(EffectComponentのLastApplied*参照)。
                // 値は毎フレーム同じ入力から算出されるため、変化がなければビット単位で
                // 一致する。epsilon比較は不要かつ「わずかな移動が反映されない」不具合の元になる。
                const bool forceApply = !effect.HasAppliedTransform;

                if (forceApply || !IsSameFloat3(worldPos, effect.LastAppliedLocation))
                {
                    effect.Effect.SetLocation(worldPos);
                    effect.LastAppliedLocation = worldPos;
                }

                if (forceApply || !IsSameFloat3(worldScale, effect.LastAppliedScale))
                {
                    effect.Effect.SetScale(worldScale);
                    effect.LastAppliedScale = worldScale;
                }

                if (forceApply || !IsSameFloat3(effect.Rotation, effect.LastAppliedRotation))
                {
                    effect.Effect.SetRotation(effect.Rotation);
                    effect.LastAppliedRotation = effect.Rotation;
                }

                effect.HasAppliedTransform = true;
            });

        // Effekseer 内部更新（秒 → フレーム換算、60fps 基準）
        mManager->Update(dt * 60.f);

        // メモリプールのフレーム更新
        mMemoryPool->NewFrame();
    }

    // -----------------------------------------------------------------------
    //  Draw
    // -----------------------------------------------------------------------
    void EffekseerManager::Draw(entt::registry& registry, ID3D12GraphicsCommandList* cmdList)
    {
        if (!mIsInitialized) return;

        auto& camSys = sys::CameraSystem::Get();
        if (!camSys.HasMainCamera()) return;

        auto* cam = registry.try_get<ecs::CameraComponent>(camSys.GetMainCameraEntity());
        if (!cam) return;

        mRenderer->SetCameraMatrix(ToEffekseerMatrix(cam->GetViewMatrix()));
        mRenderer->SetProjectionMatrix(ToEffekseerMatrix(cam->GetProjectionMatrix()));

        EffekseerRendererDX12::BeginCommandList(mCmdList, cmdList);
        mRenderer->SetCommandList(mCmdList);

        // 描画呼び出し数・頂点数はそれぞれ専用のReset関数を呼ばない限りアプリ起動からの
        // 累積値のまま増え続ける(ResetDrawCallCount()は頂点数側をリセットしない)ため、
        // 両方を毎フレームリセットしてから計測する(PerformanceMonitorでの負荷診断用)
        mRenderer->ResetDrawCallCount();
        mRenderer->ResetDrawVertexCount();

        mRenderer->BeginRendering();
        mManager->Draw();
        mRenderer->EndRendering();

        mLastDrawCallCount = mRenderer->GetDrawCallCount();
        mLastDrawVertexCount = mRenderer->GetDrawVertexCount();
        mLastInstanceCount = mManager->GetTotalInstanceCount();

        mRenderer->SetCommandList(nullptr);
        EffekseerRendererDX12::EndCommandList(mCmdList);
    }

    // -----------------------------------------------------------------------
    //  アセット管理
    // -----------------------------------------------------------------------
    Effekseer::EffectRef EffekseerManager::GetEffect(
        const std::filesystem::path& filePath)
    {
        const std::u16string key =
            std::filesystem::absolute(filePath).lexically_normal().u16string();

        auto it = mEffectCache.find(key);
        if (it != mEffectCache.end()) return it->second;

        auto effect = Effekseer::Effect::Create(mManager, key.c_str());
        if (effect == nullptr)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "EffekseerManager: Failed to load effect: {}",
                filePath.string());
            return nullptr;
        }

        mEffectCache.emplace(key, effect);
        // 素材別の負荷集計(AccumulateEffectStat)で表示するため、フルパスではなく
        // ファイル名だけを控えておく
        mEffectNames.emplace(effect.Get(), filePath.filename().string());
        return effect;
    }

    // -----------------------------------------------------------------------
    //  生成直後の非表示化
    // -----------------------------------------------------------------------
    void EffekseerManager::MarkSpawnHidden(ecs::EffectComponent& effect)
    {
        effect.Effect.SetRenderingVisible(false);
        effect.HiddenFramesRemaining = GetSpawnHiddenFrames();
    }

    // -----------------------------------------------------------------------
    //  素材別の負荷集計
    // -----------------------------------------------------------------------
    void EffekseerManager::AccumulateEffectStat(const ecs::EffectComponent& effect)
    {
        if (effect.Asset == nullptr) return;

        const Effekseer::Handle handle = effect.Effect.GetHandle();
        if (handle < 0) return;

        const int32_t instances = mManager->GetInstanceCount(handle);
        if (instances <= 0) return;

        const Effekseer::Effect* key = effect.Asset.Get();

        // 素材数は多くても数十のため線形探索で十分(mapを毎フレーム構築するより安い)
        for (auto& entry : mEffectStats)
        {
            if (entry.Asset == key)
            {
                entry.HandleCount += 1;
                entry.InstanceCount += instances;
                return;
            }
        }

        const auto nameIt = mEffectNames.find(key);
        mEffectStats.push_back(EffectStatEntry{
            key,
            nameIt != mEffectNames.end() ? &nameIt->second : nullptr,
            1,
            instances });
    }

    // -----------------------------------------------------------------------
    //  手動再生 API
    // -----------------------------------------------------------------------
    Effekseer::Handle EffekseerManager::Play(
        Effekseer::EffectRef     effect,
        const DirectX::XMFLOAT3& position,
        float                    scale)
    {
        if (!mIsInitialized || effect == nullptr) return -1;

        Effekseer::Handle handle = mManager->Play(
            effect, position.x, position.y, position.z);

        if (handle >= 0)
            mManager->SetScale(handle, scale, scale, scale);

        return handle;
    }

    void EffekseerManager::Stop(Effekseer::Handle handle)
    {
        if (!mIsInitialized || handle < 0) return;
        mManager->StopEffect(handle);
    }

    void EffekseerManager::StopAll()
    {
        if (!mIsInitialized) return;
        mManager->StopAllEffects();
    }

    // -----------------------------------------------------------------------
    //  行列変換
    // -----------------------------------------------------------------------
    Effekseer::Matrix44 EffekseerManager::ToEffekseerMatrix(
        const DirectX::XMMATRIX& mat)
    {
        DirectX::XMFLOAT4X4 m;
        DirectX::XMStoreFloat4x4(&m, mat);

        Effekseer::Matrix44 result;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                result.Values[r][c] = m.m[r][c];

        return result;
    }

} // namespace sys