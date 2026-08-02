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

    /// <summary>
    /// FBX モデルの描画を管理するクラス
    /// </summary>
    class ENGINE_API FbxRenderer : public utility::Singleton<FbxRenderer>
    {
        SINGLETON_CLASS(FbxRenderer);
    public:
        SINGLETON_ACCESSOR(FbxRenderer);

        /// <summary>
        /// パイプラインと GPU バッファを初期化する
        /// </summary>
        bool Initialize(
            DX12Device& device,
            GDescriptorHeapManager& heapManager,
            ShaderManager& shaderManager);

        /// <summary>
        /// リソースを解放する
        /// </summary>
        void Finalize();

        /// <summary>
        /// フレーム先頭でフレームデータをクリアする
        /// </summary>
        void Begin();

        /// <summary>
        /// ECS レジストリからコンポーネントを収集して描画登録する
        /// </summary>
        void UpdateAndDraw(entt::registry& registry);

        /// <summary>
        /// アニメーション距離 LOD の有効と無効を設定する
        /// </summary>
        void SetAnimationDistanceLodEnabled(bool enabled) { mAnimationDistanceLodEnabled = enabled; }

        /// <summary>
        /// アニメーション距離 LOD が有効かを取得する
        /// </summary>
        bool IsAnimationDistanceLodEnabled() const { return mAnimationDistanceLodEnabled; }

        /// <summary>
        /// ボーン計算を打ち切るカメラからの距離を設定する
        /// </summary>
        void  SetAnimationUpdateDistance(float distance) { mAnimationUpdateDistance = distance; }

        /// <summary>
        /// ボーン計算を打ち切るカメラからの距離を取得する
        /// </summary>
        float GetAnimationUpdateDistance() const { return mAnimationUpdateDistance; }

    private:
        /// <summary>
        /// 距離 LOD の切り替え用デバッグウィンドウ
        /// </summary>
        void ImGuiWindow();

    public:
        /// <summary>
        /// シャドウパスを実行する
        /// </summary>
        void DrawShadowPass(ID3D12GraphicsCommandList* cmdList);

        /// <summary>
        /// 蓄積した描画命令を GPU コマンドとして発行する
        /// </summary>
        void End(ID3D12GraphicsCommandList* cmdList);

        /// <summary>
        /// ライトパラメータを設定する
        /// </summary>
        void SetLights(const std::vector<LightData>& lights);

        /// <summary>
        /// シーンバッファの GPU ハンドルを取得する
        /// </summary>
        D3D12_GPU_DESCRIPTOR_HANDLE GetSceneBufferGpuHandle() const
        {
            return mSceneBuffer->GetGpuHandle();
        }

        /// <summary>
        /// シャドウマップの SRV GPU ハンドルを取得する
        /// </summary>
        D3D12_GPU_DESCRIPTOR_HANDLE GetShadowMapGpuHandle() const
        {
            return mShadowMapSRV.GetGpuHandle();
        }

        /// <summary>
        /// シャドウマップの解像度
        /// </summary>
        static constexpr UINT SHADOW_MAP_SIZE = 2048u;

    private:
        /// <summary>
        /// 1エンティティ分のデータをフレームバッファに積む
        /// </summary>
        void Submit(
            const FbxResource& resource,
            const DirectX::XMFLOAT4X4& world,
            const std::vector<DirectX::XMFLOAT4X4>* boneMatrices,
            const DirectX::XMFLOAT4& customColor);

        /// <summary>
        /// シャドウマップリソースを生成する
        /// </summary>
        bool CreateShadowMapResources(DX12Device& device, GDescriptorHeapManager& heapManager);

        /// <summary>
        /// 描画呼び出し情報
        /// </summary>
        struct DrawCall
        {
            const FbxResource* Resource = nullptr;
            uint32_t           SectionIndex = 0;
            uint32_t           InstanceIndex = 0;
        };

        /// <summary>
        /// 描画対象のアイテム情報
        /// </summary>
        struct RenderItem
        {
            const ecs::Transform* Transform = nullptr;
            const ecs::FbxComponent* Fbx = nullptr;
            ecs::FbxAnimComponent* Anim = nullptr;
        };

        /// <summary>
        /// 最大インスタンス数
        /// </summary>
        static constexpr uint32_t MAX_FBX_INSTANCES = 512u;

        /// <summary>
        /// 最大ボーン数
        /// </summary>
        static constexpr uint32_t MAX_TOTAL_BONES = 32768u;

        /// <summary>
        /// 最大ライト数
        /// </summary>
        static constexpr uint32_t MAX_LIGHTS = 64u;

        /// <summary>
        /// 既定の打ち切り距離
        /// </summary>
        static constexpr float DEFAULT_ANIMATION_UPDATE_DISTANCE = 60.0f;

        /// <summary>アニメーション距離 LOD 有効フラグ</summary>
        bool  mAnimationDistanceLodEnabled = false;

        /// <summary>アニメーション更新距離</summary>
        float mAnimationUpdateDistance = DEFAULT_ANIMATION_UPDATE_DISTANCE;

        /// <summary>FBX パイプライン</summary>
        std::unique_ptr<FbxPipeline>      mPipeline;

        /// <summary>シャドウパイプライン</summary>
        std::unique_ptr<ShadowPipeline>   mShadowPipeline;

        /// <summary>インスタンスバッファ</summary>
        std::unique_ptr<StructuredBuffer> mInstanceBuffer;

        /// <summary>ボーンバッファ</summary>
        std::unique_ptr<StructuredBuffer> mBoneBuffer;

        /// <summary>シーンバッファ</summary>
        std::unique_ptr<StructuredBuffer> mSceneBuffer;

        /// <summary>ライトバッファ</summary>
        std::unique_ptr<StructuredBuffer> mLightBuffer;

        /// <summary>シャドウマップリソース</summary>
        Resource          mShadowMapResource;

        /// <summary>シャドウマップ DSV ヒープ</summary>
        Heap              mShadowMapDSVHeap;

        /// <summary>シャドウマップ SRV</summary>
        GDescriptorHeap   mShadowMapSRV;

        /// <summary>ダミー SRV</summary>
        GDescriptorHeap   mShadowMapNullSRV;

        /// <summary>インスタンスデータリスト</summary>
        std::vector<FbxInstanceData>      mInstanceData;

        /// <summary>ボーンデータリスト</summary>
        std::vector<DirectX::XMFLOAT4X4> mBoneData;

        /// <summary>描画呼び出しリスト</summary>
        std::vector<DrawCall>             mDrawCalls;

        /// <summary>ライトデータリスト</summary>
        std::vector<LightData>            mLightData;

        /// <summary>描画アイテム一時バッファ</summary>
        std::vector<RenderItem>           mRenderItems;

        /// <summary>デフォルト白テクスチャ</summary>
        Texture* mDefaultWhiteTexture = nullptr;

        /// <summary>デフォルト法線テクスチャ</summary>
        Texture* mDefaultNormalTexture = nullptr;

        /// <summary>デフォルト黒テクスチャ</summary>
        Texture* mDefaultBlackTexture = nullptr;

        /// <summary>ディスクリプターヒープマネージャーへのポインタ</summary>
        GDescriptorHeapManager* mHeapManager = nullptr;
    };

} // namespace graphics