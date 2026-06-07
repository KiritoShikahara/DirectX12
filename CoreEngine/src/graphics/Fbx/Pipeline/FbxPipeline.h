#pragma once
#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
	class DX12Device;
	class ShaderManager;

	/// <summary>
	///	RootSignatureスロット (全て DescriptorTable / SRV):
	/// SLOT_INSTANCE_BUFFER (t0) : StructuredBuffer<FbxInstanceData>
	/// SLOT_BONE_BUFFER     (t1) : StructuredBuffer<float4x4>
	/// SLOT_ALBEDO_TEX      (t2) : Texture2D  ベースカラー
	/// SLOT_NORMAL_TEX      (t3) : Texture2D  法線マップ
	/// SLOT_METALLIC_TEX    (t4) : Texture2D  メタリック
	/// SLOT_ROUGHNESS_TEX   (t5) : Texture2D  ラフネス
	/// SLOT_AO_TEX          (t6) : Texture2D  AO
	/// SLOT_EMISSIVE_TEX    (t7) : Texture2D  エミッシブ
	/// SLOT_SCENE_BUFFER    (t8) : StructuredBuffer<FbxSceneData>	/// </summary>
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

		// ルートパラメータスロット番号
		static constexpr UINT SLOT_INSTANCE_BUFFER = 0; // t0 : ALL
		static constexpr UINT SLOT_BONE_BUFFER = 1; // t1 : VERTEX
		static constexpr UINT SLOT_ALBEDO_TEX = 2; // t2 : PIXEL
		static constexpr UINT SLOT_NORMAL_TEX = 3; // t3 : PIXEL
		static constexpr UINT SLOT_METALLIC_TEX = 4; // t4 : PIXEL
		static constexpr UINT SLOT_ROUGHNESS_TEX = 5; // t5 : PIXEL
		static constexpr UINT SLOT_AO_TEX = 6; // t6 : PIXEL
		static constexpr UINT SLOT_EMISSIVE_TEX = 7; // t7 : PIXEL
		static constexpr UINT SLOT_SCENE_BUFFER = 8; // t8 : ALL
		static constexpr UINT SLOT_LIGHT_BUFFER = 9; // t9 : light


	private:
		bool CreateRootSignature(ID3D12Device* device);
		bool CreatePipeline(ID3D12Device* device, ShaderManager& shaderManager);

		static D3D12_DEPTH_STENCIL_DESC MakeDepthStencilDesc();
		static D3D12_BLEND_DESC         MakeBlendDesc();
		static D3D12_RASTERIZER_DESC    MakeRasterizerDesc();

		RootSig mRootSignature = nullptr;
		PSO     mPipelineState = nullptr;

	};
}


