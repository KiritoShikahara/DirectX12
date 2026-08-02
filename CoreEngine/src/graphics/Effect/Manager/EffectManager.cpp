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
    bool IsSameFloat3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
    {
        // 厳密比較(epsilon不要、毎フレーム同一入力から算出されるため)
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
}

namespace graphics
{
    bool EffekseerManager::Initialize(
        graphics::DX12Device& device,
        graphics::DX12Context& context)
    {
        if (mIsInitialized) return false;

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

        if constexpr (EFFECT_WORKER_THREAD_COUNT > 0)
        {
            // SyncUpdate=trueのため、ワーカー使用時も呼び出し側の同期タイミングは変わらない
            mManager->LaunchWorkerThreads(EFFECT_WORKER_THREAD_COUNT);
            DEBUG_LOG(sys::eLogLevel::Log,
                "EffekseerManager: Launched {} worker threads for particle update.",
                EFFECT_WORKER_THREAD_COUNT);
        }

        mManager->SetSpriteRenderer(mRenderer->CreateSpriteRenderer());
        mManager->SetRibbonRenderer(mRenderer->CreateRibbonRenderer());
        mManager->SetRingRenderer(mRenderer->CreateRingRenderer());
        mManager->SetModelRenderer(mRenderer->CreateModelRenderer());
        mManager->SetTrackRenderer(mRenderer->CreateTrackRenderer());

        mManager->SetTextureLoader(mRenderer->CreateTextureLoader());
        mManager->SetModelLoader(mRenderer->CreateModelLoader());
        mManager->SetMaterialLoader(mRenderer->CreateMaterialLoader());

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

    void EffekseerManager::Update(entt::registry& registry, float dt)
    {
        if (!mIsInitialized) return;

        // 集計コストがあるためDebug/Developのみ実行
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
                    AccumulateEffectStat(effect);
                }

                if (effect.IsVisible != effect.LastIsVisible)
                {
                    effect.Effect.SetVisible(effect.IsVisible);
                    effect.LastIsVisible = effect.IsVisible;
                }

                if (!effect.IsVisible) return;

                // Play()の初期位置とSetLocation()の追従先を一致させるため一度だけ計算する
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

                // tick数(dt*60.f)で管理し、fps変動に依存させない
                if (effect.HiddenFramesRemaining > 0.f)
                {
                    effect.HiddenFramesRemaining -= dt * 60.f;
                    if (effect.HiddenFramesRemaining <= 0.f)
                    {
                        effect.HiddenFramesRemaining = 0.f;
                        effect.Effect.SetRenderingVisible(true);
                    }
                }

                if (!effect.Effect.IsPlaying())
                {
                    if (effect.IsLoop == true && effect.Asset != nullptr)
                    {
                        // 再始動直後も新規生成と同じ猶予を与える
                        effect.Effect.Play(effect.Asset, worldPos, effect.Effect.ShouldDestroy());
                        MarkSpawnHidden(effect);
                        effect.HasAppliedTransform = false;
                    }
                    else if (effect.HiddenFramesRemaining > 0.f)
                    {
                        // 猶予中は破棄しない
                    }
                    else
                    {
                        if (effect.Effect.ShouldDestroy())
                            registry.destroy(entity);
                        return;
                    }
                }

                const DirectX::XMFLOAT3 worldScale = targetTrans
                    ? DirectX::XMFLOAT3{
                        targetTrans->GetScale().x * effect.Scale.x,
                        targetTrans->GetScale().y * effect.Scale.y,
                        targetTrans->GetScale().z * effect.Scale.z }
                    : effect.Scale;

                // 前回適用値から変化した項目だけ呼ぶ(Setterはstd::map検索を伴うため)
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

        mManager->Update(dt * 60.f);

        mMemoryPool->NewFrame();
    }

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

        // 累積値のまま増え続けるため毎フレームリセットする
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
        // 表示用にファイル名だけ控えておく
        mEffectNames.emplace(effect.Get(), filePath.filename().string());
        return effect;
    }

    void EffekseerManager::MarkSpawnHidden(ecs::EffectComponent& effect)
    {
        effect.Effect.SetRenderingVisible(false);
        effect.HiddenFramesRemaining = GetSpawnHiddenTicks();
    }

    void EffekseerManager::AccumulateEffectStat(const ecs::EffectComponent& effect)
    {
        if (effect.Asset == nullptr) return;

        const Effekseer::Handle handle = effect.Effect.GetHandle();
        if (handle < 0) return;

        const int32_t instances = mManager->GetInstanceCount(handle);
        if (instances <= 0) return;

        const Effekseer::Effect* key = effect.Asset.Get();

        // 素材数は多くても数十のため線形探索で十分
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
