#include "pch.h"
#include"EffectManager.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Dx12/Dx12Context.h>
#include <ecs/component/effect/EffectComponent.h>
#include <ecs/component/transform/TransformComponent.h>
#include<system/Camera/CameraSystem.h>
#include<ecs/component/camera/CameraComponent.h>

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

        mManager = Effekseer::Manager::Create(MAX_SQUARES);
        if (mManager == nullptr)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                "EffekseerManager: Failed to create Manager.");
            return false;
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

        registry.view<ecs::EffectComponent>().each([&](entt::entity entity, ecs::EffectComponent& effect)
            {
                // 表示状態の変更を反映
                if (effect.IsVisible != effect.LastIsVisible)
                {
                    effect.Effect.SetVisible(effect.IsVisible);
                    effect.LastIsVisible = effect.IsVisible;
                }

                if (!effect.IsVisible) return;

                // 再生終了していたら
                if (!effect.Effect.IsPlaying())
                {
                    if (effect.IsLoop== true && effect.Asset != nullptr)
                    {
                        // ループ: 再スタート
                        effect.Effect.Play(effect.Asset, effect.Offset, effect.Effect.ShouldDestroy());
                    }
                    else
                    {
                        // 自動削除フラグがあればエンティティを削除
                        if (effect.Effect.ShouldDestroy())
                            registry.destroy(entity);
                        return;
                    }
                }

                // Transform に追従する位置・回転・スケールの更新
                ecs::Transform* targetTrans = nullptr;

                if (effect.Parent != entt::null && registry.valid(effect.Parent))
                    targetTrans = registry.try_get<ecs::Transform>(effect.Parent);
                else
                    targetTrans = registry.try_get<ecs::Transform>(entity);

                if (targetTrans)
                {
                    // 位置 = Transform.Position + Offset
                    const auto& pos = targetTrans->GetPosition();
                    DirectX::XMFLOAT3 finalPos = {
                        pos.x + effect.Offset.x,
                        pos.y + effect.Offset.y,
                        pos.z + effect.Offset.z
                    };
                    effect.Effect.SetLocation(finalPos);

                    // スケール = Transform.Scale * EffectComponent.Scale
                    const auto& trScale = targetTrans->GetScale();
                    effect.Effect.SetScale({
                        trScale.x * effect.Scale.x,
                        trScale.y * effect.Scale.y,
                        trScale.z * effect.Scale.z
                        });
                }
                else
                {
                    // Transform が無い場合は Offset を直接座標として使う
                    effect.Effect.SetLocation(effect.Offset);
                    effect.Effect.SetScale(effect.Scale);
                }

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

        mRenderer->BeginRendering();
        mManager->Draw();
        mRenderer->EndRendering();

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
        return effect;
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