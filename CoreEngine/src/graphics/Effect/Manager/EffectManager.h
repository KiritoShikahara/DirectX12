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

        /// <summary>
        /// エフェクトファイルをロード（キャッシュあり）。
        /// EffectComponent::Asset に渡す。
        /// </summary>
        Effekseer::EffectRef GetEffect(const std::filesystem::path& filePath);

        // 手動再生
        Effekseer::Handle Play(
            Effekseer::EffectRef     effect,
            const DirectX::XMFLOAT3& position,
            float                    scale = 1.f);

        void Stop(Effekseer::Handle handle);
        void StopAll();

        // 内部アクセス
        Effekseer::ManagerRef GetManager() { return mManager; }
        bool IsInitialized() const { return mIsInitialized; }

        /// <summary>
        /// 直近のDraw()で発行された描画呼び出し数(Effekseer/LLGI内部で1バッチ=1呼び出し)。
        /// PerformanceMonitorでの負荷診断用(PhysicsDebugRenderer等と異なりDraw()自体は
        /// 何も重い処理をしていないため、実際の負荷はこの回数×LLGI側の1呼び出しあたりの
        /// 固定コスト(PSO/デスクリプタ再設定)に比例する。チューニングの指標として使う)。
        /// </summary>
        int32_t GetLastDrawCallCount() const { return mLastDrawCallCount; }

        /// <summary>直近のDraw()で発行された頂点数</summary>
        int32_t GetLastDrawVertexCount() const { return mLastDrawVertexCount; }

        /// <summary>直近フレームで生存していたエフェクトインスタンス(Handle)数</summary>
        int32_t GetLastInstanceCount() const { return mLastInstanceCount; }

        /// <summary>
        /// パーティクル更新(Manager::Update)を分散させるワーカースレッド数。0で無効。
        ///
        /// Effekseerの並列化はDoUpdate内でインスタンスをチャンク分割して行うため、
        /// 実際に並列化されるにはワーカーが2以上必要(Effekseer.Manager.cpp参照)。
        /// Update()自体はSyncUpdate=true(既定)なので完了を待って返る。つまり
        /// 呼び出し側から見た同期のタイミングは変わらず、余分な遅延も発生しない。
        ///
        /// 無効化したい場合はここを0にすること(以前ワーカースレッド有効化時に
        /// エフェクトの表示崩れが発生した経緯があるため、切り分け用に残している)。
        /// </summary>
        static constexpr uint32_t EFFECT_WORKER_THREAD_COUNT = 8;

        /// <summary>
        /// 再生開始直後にエフェクトを非表示にしておくフレーム数。
        /// 生成直後はビルボードの向き等が未確定で素の四角形に見えるため、その間を隠す。
        /// ワーカースレッド有効時は内部状態の確定が1フレーム余分にかかりうるため多めに取る
        /// (ecs::EffectComponent::HiddenFramesRemaining参照)。
        /// </summary>
        static constexpr int GetSpawnHiddenFrames()
        {
            return (EFFECT_WORKER_THREAD_COUNT > 0) ? 2 : 1;
        }

        /// <summary>
        /// エフェクト素材(.efk)1種類あたりの負荷内訳。総インスタンス数だけでは
        /// 「どの素材を削れば効くか」が分からないため、素材単位で集計する。
        /// </summary>
        struct EffectStatEntry
        {
            const Effekseer::Effect* Asset = nullptr;
            /// <summary>素材のファイル名(mEffectNames内の文字列を指す。寿命はmEffectNamesと同じ)</summary>
            const std::string* Name = nullptr;
            /// <summary>この素材を再生中のHandle数(=同時再生数)</summary>
            int32_t HandleCount = 0;
            /// <summary>この素材が生成しているパーティクルインスタンスの合計</summary>
            int32_t InstanceCount = 0;
        };

        /// <summary>
        /// 直近のUpdate()で集計した素材別の負荷内訳。
        /// 集計自体がコストになるためデバッグ/開発ビルドでのみ更新される(Releaseでは空)。
        /// </summary>
        const std::vector<EffectStatEntry>& GetEffectStats() const { return mEffectStats; }

    private:
        /// <summary>1エンティティ分の負荷をmEffectStatsへ加算する(素材単位でまとめる)</summary>
        void AccumulateEffectStat(const ecs::EffectComponent& effect);

        static Effekseer::Matrix44 ToEffekseerMatrix(const DirectX::XMMATRIX& mat);

        Effekseer::ManagerRef          mManager;
        EffekseerRenderer::RendererRef mRenderer;

        Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> mMemoryPool;
        Effekseer::RefPtr<EffekseerRenderer::CommandList>           mCmdList;

        std::unordered_map<std::u16string, Effekseer::EffectRef> mEffectCache;

        /// <summary>
        /// 素材の表示名(ファイル名のみ)。EffectStatEntry::Nameが指す実体であり、
        /// unordered_mapの要素アドレスは再ハッシュでも不変なためポインタ保持して安全。
        /// </summary>
        std::unordered_map<const Effekseer::Effect*, std::string> mEffectNames;

        /// <summary>
        /// 素材別の負荷内訳。毎フレームclear()して再利用する(容量は保持されるため
        /// 定常状態では再確保が起きない)。
        /// </summary>
        std::vector<EffectStatEntry> mEffectStats;

        /// <summary>
        /// Effekseerが同時に保持できるパーティクルインスタンス数の上限。
        ///
        /// この上限に達すると新規パーティクルが生成できず、エフェクトが欠けた状態で
        /// 描画される・そもそも表示されないという症状になる。
        /// 旧値8000はRelease実測のピーク7,665(=96%)に対して余裕がなく、枯渇していた。
        /// 実測ではインスタンス約7,600でもUpdate/Drawは合計1ms未満(Release)であり、
        /// 上限を上げてもフレームレートへの影響はほぼ無いため、十分な余裕を持たせる。
        ///
        /// 注意: 以前「この上限が負荷の頭打ちとして機能するので増やすと悪化する」と
        /// 判断していたが、その根拠にしていた計測値(Update 11ms)はDebugビルドのもので、
        /// Release実測とは数十倍の乖離があった。負荷判断は必ずRelease構成で行うこと。
        /// </summary>
        static constexpr int32_t MAX_INSTANCES = 24000;

        /// <summary>
        /// 1フレームに描画できるスプライト(板ポリ)数の上限。頂点バッファの容量そのものであり、
        /// MAX_INSTANCESとは役割が異なるため別定数として持つ。
        ///
        /// EffekseerRendererLLGIの頂点バッファはリングバッファだが、実体はringVertexCount_=3個
        /// しかない(EffekseerRendererLLGI.VertexBuffer.h)。1フレームの描画スプライト数が
        /// この上限を超えるとリングが1周し、physicalバッファを使い回し始める。3周を超えると
        /// 「GPUがまだ読んでいない頂点データをCPUが同一フレーム内で上書きする」状態になり、
        /// エフェクトが1フレームだけ崩れて見える描画バグの原因になる。
        ///
        /// 実測(戦闘時)で約52,000スプライト/フレームに達しており、旧値8000では1フレームに
        /// 6.5周してしまい確実に破壊が起きていた。MAX_INSTANCES上限まで出た場合の
        /// 最悪ケース(約72,000スプライト)でも周回数が3未満に収まる値にしておく。
        /// メモリコストは 72byte(DynamicVertexWithCustomData) * 本値 * 4頂点 * (GPU3枚+CPU1枚)。
        /// </summary>
        static constexpr int32_t MAX_SQUARES = 32768;

        bool mIsInitialized = false;

        int32_t mLastDrawCallCount = 0;
        int32_t mLastDrawVertexCount = 0;
        int32_t mLastInstanceCount = 0;
    };

} // namespace sys