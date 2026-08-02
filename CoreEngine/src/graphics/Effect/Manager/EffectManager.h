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
#include <vector>

namespace ecs { struct EffectComponent; }

namespace graphics
{

    class DX12Device;
    class DX12Context;

    ///<summary>Effekseerの初期化・更新・描画を管理するシングルトン</summary>
    class ENGINE_API EffekseerManager : public utility::Singleton<EffekseerManager>
    {
        SINGLETON_CLASS(EffekseerManager);
    public:
        SINGLETON_ACCESSOR(EffekseerManager);

        bool Initialize(graphics::DX12Device& device, graphics::DX12Context& context);
        void Finalize();

        ///<summary>ECSのEffectComponentを収集して更新する</summary>
        void Update(entt::registry& registry, float dt);

        ///<summary>登録済みエフェクトを描画する。メインカメラが無ければ何もしない</summary>
        void Draw(entt::registry& registry, ID3D12GraphicsCommandList* cmdList);

        ///<summary>エフェクトファイルをロードする(キャッシュあり)</summary>
        Effekseer::EffectRef GetEffect(const std::filesystem::path& filePath);

        Effekseer::Handle Play(
            Effekseer::EffectRef     effect,
            const DirectX::XMFLOAT3& position,
            float                    scale = 1.f);

        void Stop(Effekseer::Handle handle);
        void StopAll();

        Effekseer::ManagerRef GetManager() { return mManager; }
        bool IsInitialized() const { return mIsInitialized; }

        ///<summary>直近のDraw()での描画呼び出し数</summary>
        int32_t GetLastDrawCallCount() const { return mLastDrawCallCount; }

        ///<summary>直近のDraw()で発行された頂点数</summary>
        int32_t GetLastDrawVertexCount() const { return mLastDrawVertexCount; }

        ///<summary>直近フレームで生存していたエフェクトインスタンス(Handle)数</summary>
        int32_t GetLastInstanceCount() const { return mLastInstanceCount; }

        ///<summary>パーティクル更新を並列化するワーカースレッド数。0で無効化できる</summary>
        static constexpr uint32_t EFFECT_WORKER_THREAD_COUNT = 8;

        ///<summary>生成直後にエフェクトを隠しておく時間(tick数、60fps基準)</summary>
        static constexpr float GetSpawnHiddenTicks()
        {
            return (EFFECT_WORKER_THREAD_COUNT > 0) ? 2.0f : 1.0f;
        }

        ///<summary>effect.Effect.Play()の直後に必ず呼ぶこと。生成直後の見た目崩れを隠す</summary>
        static void MarkSpawnHidden(ecs::EffectComponent& effect);

        ///<summary>素材(.efk)1種類あたりの負荷内訳</summary>
        struct EffectStatEntry
        {
            const Effekseer::Effect* Asset = nullptr;
            ///<summary>素材のファイル名</summary>
            const std::string* Name = nullptr;
            ///<summary>この素材を再生中のHandle数</summary>
            int32_t HandleCount = 0;
            ///<summary>この素材が生成しているパーティクルインスタンスの合計</summary>
            int32_t InstanceCount = 0;
        };

        ///<summary>直近のUpdate()で集計した素材別の負荷内訳(Debug/Developのみ更新)</summary>
        const std::vector<EffectStatEntry>& GetEffectStats() const { return mEffectStats; }

    private:
        ///<summary>1エンティティ分の負荷をmEffectStatsへ加算する</summary>
        void AccumulateEffectStat(const ecs::EffectComponent& effect);

        static Effekseer::Matrix44 ToEffekseerMatrix(const DirectX::XMMATRIX& mat);

        Effekseer::ManagerRef          mManager;
        EffekseerRenderer::RendererRef mRenderer;

        Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> mMemoryPool;
        Effekseer::RefPtr<EffekseerRenderer::CommandList>           mCmdList;

        std::unordered_map<std::u16string, Effekseer::EffectRef> mEffectCache;

        ///<summary>素材の表示名(ファイル名)</summary>
        std::unordered_map<const Effekseer::Effect*, std::string> mEffectNames;

        ///<summary>素材別の負荷内訳。毎フレームclear()して再利用する</summary>
        std::vector<EffectStatEntry> mEffectStats;

        ///<summary>同時パーティクルインスタンス数の上限(Release実測に基づく余裕値)</summary>
        static constexpr int32_t MAX_INSTANCES = 24000;

        ///<summary>1フレームに描画できるスプライト数の上限(頂点リングバッファの周回対策)</summary>
        static constexpr int32_t MAX_SQUARES = 32768;

        bool mIsInitialized = false;

        int32_t mLastDrawCallCount = 0;
        int32_t mLastDrawVertexCount = 0;
        int32_t mLastInstanceCount = 0;
    };

} // namespace sys
