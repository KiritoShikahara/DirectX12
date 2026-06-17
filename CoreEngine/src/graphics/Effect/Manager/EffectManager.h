#pragma once

#include <Effekseer.h>
#include<EffekseerRendererDX12/EffekseerRendererDX12.h>

#include <Utility/Singleton/Singleton.hpp>
#include <Utility/Export/Export.h>
#include <graphics/Dx12/Dx12Type.h>

#include <DirectXMath.h>
#include <entt/entt.hpp>
#include <filesystem>
#include <unordered_map>
#include <string>

namespace graphics
{

    class DX12Device;
    class DX12Context;


    /// <summary>
    /// Effekseer の初期化・更新・描画を管理するシングルトン。
    ///
    /// フレームの流れ:
    ///   Engine::Update() → EffekseerManager::Get().Update(registry, dt)
    ///   Engine::Render() → EffekseerManager::Get().Draw(cmdList, view, proj)
    /// </summary>
    class ENGINE_API EffekseerManager : public utility::Singleton<EffekseerManager>
    {
        SINGLETON_CLASS(EffekseerManager);
    public:
        SINGLETON_ACCESSOR(EffekseerManager);

        bool Initialize(graphics::DX12Device& device, graphics::DX12Context& context);
        void Finalize();

        // -----------------------------------------------------------------------
        //  フレーム API
        // -----------------------------------------------------------------------

        /// <summary>
        /// ECS から EffectComponent を収集して更新する。
        /// Engine::Update() 内で呼ぶ。
        /// </summary>
        void Update(entt::registry& registry, float dt);

        /// <summary>
        /// エフェクトを描画する。
        /// メインカメラの View/Proj を内部で取得するため、
        /// 呼び出し側はレジストリと cmdList を渡すだけでよい。
        /// メインカメラが存在しない場合は何もしない。
        /// Engine::Render() の FbxRenderer::End() 後に呼ぶ。
        /// </summary>
        void Draw(entt::registry& registry, ID3D12GraphicsCommandList* cmdList);

        // -----------------------------------------------------------------------
        //  アセット管理
        // -----------------------------------------------------------------------

        /// <summary>
        /// エフェクトファイルをロード（キャッシュあり）。
        /// EffectComponent::Asset に渡す。
        /// </summary>
        Effekseer::EffectRef GetEffect(const std::filesystem::path& filePath);

        // -----------------------------------------------------------------------
        //  手動再生 API（コードから直接再生したい場合）
        // -----------------------------------------------------------------------

        Effekseer::Handle Play(
            Effekseer::EffectRef     effect,
            const DirectX::XMFLOAT3& position,
            float                    scale = 1.f);

        void Stop(Effekseer::Handle handle);
        void StopAll();

        // -----------------------------------------------------------------------
        //  内部アクセス（EffectObject から使用）
        // -----------------------------------------------------------------------
        Effekseer::ManagerRef GetManager() { return mManager; }

        bool IsInitialized() const { return mIsInitialized; }

    private:
        static Effekseer::Matrix44 ToEffekseerMatrix(const DirectX::XMMATRIX& mat);

        Effekseer::ManagerRef          mManager;
        EffekseerRenderer::RendererRef mRenderer;

        Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> mMemoryPool;
        Effekseer::RefPtr<EffekseerRenderer::CommandList>           mCmdList;

        std::unordered_map<std::u16string, Effekseer::EffectRef> mEffectCache;

        static constexpr int32_t MAX_SQUARES = 8000;

        bool mIsInitialized = false;
    };

} // namespace sys