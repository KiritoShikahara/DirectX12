#pragma once

#include <graphics/Dx12/Dx12Type.h>

namespace graphics
{
    class DX12Device;
    class ShaderManager;

    /// <summary>
    /// スカイボックス描画用 RootSignature / PSO。
    ///
    /// RootSignature スロット:
    ///   SLOT_BLEND_WEIGHT  (b0, space0) : Root32BitConstants — BlendWeight (float 1個)
    ///   SLOT_SCENE_BUFFER  (t8, space0) : DescriptorTable    — FbxSceneData (VERTEX)
    ///   SLOT_SKYBOX_TEX_A  (t0, space1) : DescriptorTable    — TextureCube A (PIXEL)
    ///   SLOT_SKYBOX_TEX_B  (t1, space1) : DescriptorTable    — TextureCube B (PIXEL)
    ///
    /// StaticSampler [s0]: LinearWrap (RootSig 埋め込み)
    ///
    /// ジオメトリ: SV_VertexID ベースのフルスクリーントライアングル (VB なし)
    /// 深度:      DepthWrite=OFF / DepthFunc=LESS_EQUAL (最遠面に描画)
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

        // ルートパラメータスロット番号 (FbxPipeline.h と同じ命名規則)
        static constexpr UINT SLOT_BLEND_WEIGHT = 0; // b0 space0 : Root32BitConstants (float 1個)
        static constexpr UINT SLOT_SCENE_BUFFER = 1; // t8 space0 : FbxSceneData (VERTEX)
        static constexpr UINT SLOT_SKYBOX_TEX_A = 2; // t0 space1 : TextureCube A (PIXEL)
        static constexpr UINT SLOT_SKYBOX_TEX_B = 3; // t1 space1 : TextureCube B (PIXEL)

    private:
        bool CreateRootSignature(ID3D12Device* device);
        bool CreatePipeline(ID3D12Device* device);

        RootSig mRootSignature = nullptr;
        PSO     mPipelineState = nullptr;
    };

} // namespace graphics