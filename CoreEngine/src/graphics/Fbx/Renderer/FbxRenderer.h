#pragma once

#include <memory>
#include <vector>
#include <d3d12.h>
#include <entt/entt.hpp>
#include <DirectXMath.h>
#include <Utility/Singleton/Singleton.hpp>

#include <graphics/FBX/Pipeline/FbxPipeline.h>
#include <graphics/FBX/Data/FbxData.h>
#include <graphics/StructuredBuffer/StructuredBuffer.h>

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

        /// <summary>フレーム先頭でフレームデータをクリアする</summary>
        void Begin();

        /// <summary>ECSレジストリからFbxComponentを収集して描画登録する</summary>
        void UpdateAndDraw(entt::registry& registry);

        /// <summary>蓄積したDrawCallをGPUコマンドとして発行する</summary>
        void End(ID3D12GraphicsCommandList* cmdList);


        /// <summary>
        /// 指向性ライトのパラメータを設定する
        /// ※ Begin() の前に毎フレーム呼ぶこと
        /// </summary>
        void SetLights(const std::vector<LightData>& lights);


        D3D12_GPU_DESCRIPTOR_HANDLE GetSceneBufferGpuHandle() const
        {
            return mSceneBuffer->GetGpuHandle();
        }

    private:
        /// <summary>
        /// 1エンティティ分のデータをフレームバッファに積む
        /// セクション数分の DrawCall が生成される
        /// </summary>
        void Submit(
            const FbxResource& resource,
            const DirectX::XMFLOAT4X4& world,
            const std::vector<DirectX::XMFLOAT4X4>* boneMatrices,
            const DirectX::XMFLOAT4& customColor);

        // ── DrawCall 単位 ────────────────────────────────────────
        struct DrawCall
        {
            const FbxResource* Resource = nullptr;
            uint32_t           SectionIndex = 0;
            uint32_t           InstanceIndex = 0;  // InstanceBuffer 内インデックス
        };

        // ── 上限定数 ─────────────────────────────────────────────
        static constexpr uint32_t MAX_FBX_INSTANCES = 512u;
        static constexpr uint32_t MAX_TOTAL_BONES = 32768u;

        // ── GPU オブジェクト ──────────────────────────────────────
        std::unique_ptr<FbxPipeline>    mPipeline;
        std::unique_ptr<StructuredBuffer> mInstanceBuffer; // FbxInstanceData[]
        std::unique_ptr<StructuredBuffer> mBoneBuffer;     // float4x4[]  (全エンティティ分を連結)
        std::unique_ptr<StructuredBuffer> mSceneBuffer;    // FbxSceneData[1]

        // ── フレームデータ (Begin でクリア) ──────────────────────
        std::vector<FbxInstanceData>      mInstanceData;
        std::vector<DirectX::XMFLOAT4X4> mBoneData;
        std::vector<DrawCall>             mDrawCalls;

        // ── デフォルトテクスチャ (nullptr スロットのフォールバック) ──
        Texture* mDefaultWhiteTexture = nullptr; // Albedo / Metallic / Roughness / AO 用
        Texture* mDefaultNormalTexture = nullptr; // Normal マップ用 (RGB=0.5,0.5,1.0)
        Texture* mDefaultBlackTexture = nullptr; // Emissive 用

        // ── ライトデータ ─────────────────────────────────────────
        std::unique_ptr<StructuredBuffer> mLightBuffer;   // LightData[] (t9)
        std::vector<LightData>            mLightData;
        static constexpr uint32_t         MAX_LIGHTS = 64u;

        // ── 依存 ─────────────────────────────────────────────────
        GDescriptorHeapManager* mHeapManager = nullptr;
    };

}


