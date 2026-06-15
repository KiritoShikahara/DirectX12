#pragma once
#include <graphics/Dx12/Dx12Type.h>

namespace graphics
{
    class DX12Device;
    class ShaderManager;

    /// <summary>
    /// RootSignature スロット:
    ///   SLOT_INSTANCE_INDEX       b0  Root32BitConstant  インスタンスインデックス
    ///   SLOT_INSTANCE_BUFFER      t0  ALL   StructuredBuffer<FbxInstanceData>
    ///   SLOT_BONE_BUFFER          t1  VS    StructuredBuffer<FbxBoneMatrix>
    ///   SLOT_ALBEDO_TEX           t2  PS    Texture2D
    ///   SLOT_NORMAL_TEX           t3  PS    Texture2D
    ///   SLOT_METALLIC_TEX         t4  PS    Texture2D
    ///   SLOT_ROUGHNESS_TEX        t5  PS    Texture2D
    ///   SLOT_AO_TEX               t6  PS    Texture2D
    ///   SLOT_EMISSIVE_TEX         t7  PS    Texture2D
    ///   SLOT_SCENE_BUFFER         t8  ALL   StructuredBuffer<FbxSceneData>
    ///   SLOT_LIGHT_BUFFER         t9  ALL   StructuredBuffer<LightData>
    ///   SLOT_SHADOW_MAP           t10 PS    Texture2D<float>  (Shadow Map)
    ///   SLOT_SHADOW_LIGHT_INDEX   b1  VS    Root32BitConstant (Shadow Pass 専用)
    ///
    /// Static Sampler:
    ///   s0  LinearSampler          (通常テクスチャ用)
    ///   s1  ShadowSampler          (PCF 比較用 COMPARISON_LESS_EQUAL)
    /// </summary>
    class FbxPipeline
    {
    public:
        FbxPipeline() = default;
        ~FbxPipeline() = default;

        FbxPipeline(const FbxPipeline&) = delete;
        FbxPipeline& operator=(const FbxPipeline&) = delete;
        FbxPipeline(FbxPipeline&&) = delete;
        FbxPipeline& operator=(FbxPipeline&&) = delete;

        bool Create(DX12Device& device, ShaderManager& shaderManager);

        ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return mPipelineState.Get(); }

        // ── ルートパラメータスロット番号 ────────────────────────
        static constexpr UINT SLOT_INSTANCE_INDEX = 0; // b0 Root32BitConstant
        static constexpr UINT SLOT_INSTANCE_BUFFER = 1; // t0 ALL
        static constexpr UINT SLOT_BONE_BUFFER = 2; // t1 VERTEX
        static constexpr UINT SLOT_ALBEDO_TEX = 3; // t2 PIXEL
        static constexpr UINT SLOT_NORMAL_TEX = 4; // t3 PIXEL
        static constexpr UINT SLOT_METALLIC_TEX = 5; // t4 PIXEL
        static constexpr UINT SLOT_ROUGHNESS_TEX = 6; // t5 PIXEL
        static constexpr UINT SLOT_AO_TEX = 7; // t6 PIXEL
        static constexpr UINT SLOT_EMISSIVE_TEX = 8; // t7 PIXEL
        static constexpr UINT SLOT_SCENE_BUFFER = 9; // t8 ALL
        static constexpr UINT SLOT_LIGHT_BUFFER = 10; // t9 ALL
        static constexpr UINT SLOT_SHADOW_MAP = 11; // t10 PIXEL
        static constexpr UINT SLOT_SHADOW_LIGHT_INDEX = 12; // b1 Root32BitConstant (Shadow Pass)

    private:
        bool CreateRootSignature(ID3D12Device* device);
        bool CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager);

        RootSig mRootSignature = nullptr;
        PSO     mPipelineState = nullptr;
    };

} // namespace graphics
