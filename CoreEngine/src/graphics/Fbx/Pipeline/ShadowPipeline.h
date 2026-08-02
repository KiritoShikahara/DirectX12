#pragma once
#include <graphics/Dx12/Dx12Type.h>

namespace graphics
{
    class DX12Device;
    class ShaderManager;

    /// <summary>
    /// Shadow Map 生成パス専用パイプライン
    /// </summary>
    class ShadowPipeline
    {
    public:
        ShadowPipeline() = default;
        ~ShadowPipeline() = default;

        ShadowPipeline(const ShadowPipeline&) = delete;
        ShadowPipeline& operator=(const ShadowPipeline&) = delete;

        /// <summary>
        /// PSO を生成する。Root Signature は FbxPipeline から受け取る。
        /// </summary>
        bool Create(
            DX12Device& device,
            ShaderManager& shaderManager,
            ID3D12RootSignature* sharedRootSignature);

        ID3D12PipelineState* GetPipelineState() const { return mPipelineState.Get(); }

        // Shadow Pass 専用スロット
        static constexpr UINT SLOT_SHADOW_LIGHT_INDEX = 11; // b1 : Root32BitConstant

    private:
        PSO mPipelineState = nullptr;
    };

} // namespace graphics
