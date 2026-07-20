#pragma once

#include <memory>
#include <vector>
#include <d3d12.h>
#include <entt/entt.hpp>
#include <DirectXMath.h>
#include <Utility/Singleton/Singleton.hpp>

#include <graphics/FBX/Pipeline/FbxPipeline.h>
#include <graphics/FBX/Pipeline/ShadowPipeline.h>
#include <graphics/FBX/Data/FbxData.h>
#include <graphics/StructuredBuffer/StructuredBuffer.h>
#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>

namespace ecs
{
    struct Transform;
    struct FbxComponent;
    struct FbxAnimComponent;
}

namespace graphics
{
    class DX12Device;
    class GDescriptorHeapManager;
    class ShaderManager;
    class FbxResource;
    class Texture;

    class FbxRenderer : public utility::Singleton<FbxRenderer>
    {
        SINGLETON_CLASS(FbxRenderer);
    public:
        SINGLETON_ACCESSOR(FbxRenderer);

        /// <summary>パイプラインとGPUバッファを初期化する</summary>
        bool Initialize(
            DX12Device& device,
            GDescriptorHeapManager& heapManager,
            ShaderManager& shaderManager);


        /// <summary>Resourceの開放</summary>
        void Finalize();

        /// <summary>フレーム先頭でフレームデータをクリアする</summary>
        void Begin();

        /// <summary>ECSレジストリからFbxComponentを収集して描画登録する</summary>
        void UpdateAndDraw(entt::registry& registry);

        /// <summary>
        /// アニメーション距離LODの有効/無効。既定は無効。
        ///
        /// 有効にすると、カメラからGetAnimationUpdateDistance()を超えた位置にある
        /// スキニングエンティティのボーン行列再計算(CalcBoneMatrices)を間引き、
        /// 最後に計算した姿勢のまま描画する。CalcBoneMatricesはボーンごとに
        /// XMMatrixDecomposeを呼ぶ重い処理のため、多数のキャラクターが遠方に存在する
        /// 状況でのCPU負荷を抑えられる。
        ///
        /// 既定で無効にしているのは、遠方のアニメーションが停止して見える副作用がある一方、
        /// 現状のコンテンツ規模ではFBXの描画コストが実測0.1ms未満で効果が無いため。
        /// キャラクター数が大幅に増えて実測でボトルネックになった場合にのみ有効化すること。
        /// </summary>
        void SetAnimationDistanceLodEnabled(bool enabled) { mAnimationDistanceLodEnabled = enabled; }
        bool IsAnimationDistanceLodEnabled() const { return mAnimationDistanceLodEnabled; }

        /// <summary>距離LOD有効時に、ボーン計算を打ち切るカメラからの距離(m)</summary>
        void  SetAnimationUpdateDistance(float distance) { mAnimationUpdateDistance = distance; }
        float GetAnimationUpdateDistance() const { return mAnimationUpdateDistance; }

    private:
        /// <summary>距離LODの切り替え用デバッグウィンドウ(開発ツール有効時のみ登録する)</summary>
        void ImGuiWindow();

    public:

        /// <summary>
        /// Shadow Pass を実行する。
        /// BeginFrame の後、通常描画パスの前に呼ぶこと。
        /// CastShadow == true の Directional Light が存在しない場合は何もしない。
        /// </summary>
        void DrawShadowPass(ID3D12GraphicsCommandList* cmdList);

        /// <summary>蓄積したDrawCallをGPUコマンドとして発行する</summary>
        void End(ID3D12GraphicsCommandList* cmdList);

        /// <summary>
        /// ライトパラメータを設定する。
        /// Begin() の前に毎フレーム呼ぶこと。
        /// LightData::CastShadow == 1 かつ Type == Directional の index 0 が Shadow 対象。
        /// </summary>
        void SetLights(const std::vector<LightData>& lights);

        D3D12_GPU_DESCRIPTOR_HANDLE GetSceneBufferGpuHandle() const
        {
            return mSceneBuffer->GetGpuHandle();
        }

        /// <summary>Shadow Map の SRV GPU ハンドルを返す (デバッグ表示用)</summary>
        D3D12_GPU_DESCRIPTOR_HANDLE GetShadowMapGpuHandle() const
        {
            return mShadowMapSRV.GetGpuHandle();
        }

        // Shadow Map 解像度
        static constexpr UINT SHADOW_MAP_SIZE = 2048u;

    private:
        /// <summary>1エンティティ分のデータをフレームバッファに積む</summary>
        void Submit(
            const FbxResource& resource,
            const DirectX::XMFLOAT4X4& world,
            const std::vector<DirectX::XMFLOAT4X4>* boneMatrices,
            const DirectX::XMFLOAT4& customColor);

        /// <summary>Shadow Map リソース・DSV・SRV を生成する</summary>
        bool CreateShadowMapResources(DX12Device& device, GDescriptorHeapManager& heapManager);

        // ── DrawCall 単位 ────────────────────────────────────────
        struct DrawCall
        {
            const FbxResource* Resource = nullptr;
            uint32_t           SectionIndex = 0;
            uint32_t           InstanceIndex = 0;
        };

        // ── UpdateAndDraw() 内で収集する描画対象1件分 ───────────────
        struct RenderItem
        {
            const ecs::Transform*    Transform = nullptr;
            const ecs::FbxComponent* Fbx = nullptr;
            ecs::FbxAnimComponent*   Anim = nullptr;
        };

        // ── 上限定数 ─────────────────────────────────────────────
        static constexpr uint32_t MAX_FBX_INSTANCES = 512u;
        static constexpr uint32_t MAX_TOTAL_BONES = 32768u;
        static constexpr uint32_t MAX_LIGHTS = 64u;

        /// <summary>アニメーション距離LODを有効にした場合の既定の打ち切り距離(m)</summary>
        static constexpr float DEFAULT_ANIMATION_UPDATE_DISTANCE = 60.0f;

        // アニメーション距離LOD設定(既定は無効。SetAnimationDistanceLodEnabled参照)
        bool  mAnimationDistanceLodEnabled = false;
        float mAnimationUpdateDistance = DEFAULT_ANIMATION_UPDATE_DISTANCE;

        // ── GPU オブジェクト ──────────────────────────────────────
        std::unique_ptr<FbxPipeline>      mPipeline;
        std::unique_ptr<ShadowPipeline>   mShadowPipeline;
        std::unique_ptr<StructuredBuffer> mInstanceBuffer;
        std::unique_ptr<StructuredBuffer> mBoneBuffer;
        std::unique_ptr<StructuredBuffer> mSceneBuffer;
        std::unique_ptr<StructuredBuffer> mLightBuffer;

        // ── Shadow Map リソース ───────────────────────────────────
        Resource          mShadowMapResource;   // D32_FLOAT テクスチャ
        Heap              mShadowMapDSVHeap;    // DSV 専用ヒープ (非シェーダービジブル)
        GDescriptorHeap   mShadowMapSRV;        // GDescriptorHeapManager 管理の SRV スロット
        GDescriptorHeap   mShadowMapNullSRV;    // Shadow Map なし時のダミー SRV

        // ── フレームデータ ────────────────────────────────────────
        std::vector<FbxInstanceData>      mInstanceData;
        std::vector<DirectX::XMFLOAT4X4> mBoneData;
        std::vector<DrawCall>             mDrawCalls;
        std::vector<LightData>            mLightData;

        // UpdateAndDraw()の一時バッファ。毎フレームclear()して再利用する(毎フレームのnew/vector生成禁止のため)
        std::vector<RenderItem>           mRenderItems;

        // ── デフォルトテクスチャ ──────────────────────────────────
        Texture* mDefaultWhiteTexture = nullptr;
        Texture* mDefaultNormalTexture = nullptr;
        Texture* mDefaultBlackTexture = nullptr;

        // ── 依存 ─────────────────────────────────────────────────
        GDescriptorHeapManager* mHeapManager = nullptr;
    };

} // namespace graphics
