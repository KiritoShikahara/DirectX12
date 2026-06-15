#pragma once
#include <graphics/Dx12/Dx12Type.h>

namespace graphics
{
    class DX12Device;
    class ShaderManager;

    /// <summary>
    /// Shadow Map 生成パス専用パイプライン
    ///
    /// Root Signature は FbxPipeline と共有する。
    /// PSO はDepthOnly (PS なし)、Shadow Map のサイズに合わせた
    /// DepthBias を適用してアクネを軽減する。
    ///
    /// ルートパラメータスロット (FbxPipeline と同一レイアウト):
    ///   SLOT_INSTANCE_INDEX  (b0) : Root32BitConstant  インスタンスインデックス
    ///   SLOT_INSTANCE_BUFFER (t0) : InstanceBuffer
    ///   SLOT_BONE_BUFFER     (t1) : BoneBuffer
    ///   SLOT_LIGHT_BUFFER    (t9) : LightBuffer  (VS_Shadow が LightViewProj を参照)
    ///
    /// Shadow Pass 専用定数:
    ///   SLOT_SHADOW_LIGHT_INDEX (b1) : Root32BitConstant  ライトインデックス
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

        // Shadow Pass 専用スロット (FbxPipeline のスロット番号と同じレイアウトを使う)
        static constexpr UINT SLOT_SHADOW_LIGHT_INDEX = 11; // b1 : Root32BitConstant

    private:
        PSO mPipelineState = nullptr;
    };

} // namespace graphics
