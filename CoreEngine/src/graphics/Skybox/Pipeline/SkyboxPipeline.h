#pragma once

#include <graphics/Dx12/Dx12Type.h>

namespace graphics
{
    class DX12Device;
    class ShaderManager;

    /// <summary>
    /// スカイボックス描画用
    /// </summary>
    class SkyboxPipeline
    {
    public:
        SkyboxPipeline() = default;
        ~SkyboxPipeline() = default;

        SkyboxPipeline(const SkyboxPipeline&) = delete;
        SkyboxPipeline& operator=(const SkyboxPipeline&) = delete;
        SkyboxPipeline(SkyboxPipeline&&) = delete;
        SkyboxPipeline& operator=(SkyboxPipeline&&) = delete;

        bool Create();

        ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return mPipelineState.Get(); }

        // ルートパラメータスロット番号
        static constexpr UINT SLOT_BLEND_WEIGHT = 0; // b0 space0 : Root32BitConstants
        static constexpr UINT SLOT_SCENE_BUFFER = 1; // t8 space0 : FbxSceneData
        static constexpr UINT SLOT_SKYBOX_TEX_A = 2; // t0 space1 : TextureCube A
        static constexpr UINT SLOT_SKYBOX_TEX_B = 3; // t1 space1 : TextureCube B

    private:
        bool CreateRootSignature(ID3D12Device* device);
        bool CreatePipeline(ID3D12Device* device);

        RootSig mRootSignature = nullptr;
        PSO     mPipelineState = nullptr;
    };

} // namespace graphics