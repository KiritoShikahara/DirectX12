#pragma once

#include <graphics/Dx12/Dx12Type.h>
#include <Utility/Export/Export.h>

namespace graphics
{
    class DX12Device;
    class ShaderManager;

    /// <summary>
    /// テキスト描画用 RootSignature / PSO
    ///
    /// RootSignature スロット:
    ///   [0] DescriptorTable CBV b0 : TextSceneData  (ScreenW/H, PxRange, Threshold)
    ///   [1] DescriptorTable SRV t0 : MSDF アトラステクスチャ
    ///   [2] Root32BitConstants  b1 : RGBA カラー (4 floats)
    ///
    /// StaticSampler s0: LinearClamp
    /// </summary>
    class ENGINE_API TextPipeline
    {
    public:
        TextPipeline() = default;
        ~TextPipeline() = default;

        TextPipeline(const TextPipeline&) = delete;
        TextPipeline& operator=(const TextPipeline&) = delete;

        bool Create(DX12Device& device, ShaderManager& shaderManager);

        ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return mPipelineState.Get(); }

        // スロット番号定数
        static constexpr UINT SLOT_SCENE_CBV = 0;  // b0
        static constexpr UINT SLOT_ATLAS_SRV = 1;  // t0
        static constexpr UINT SLOT_COLOR = 2;  // b1 (Root32BitConstants)

    private:
        RootSig mRootSignature;
        PSO     mPipelineState;
    };

} // namespace graphics