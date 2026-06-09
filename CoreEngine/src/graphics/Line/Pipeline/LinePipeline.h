#pragma once

#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
	class LinePipeline
	{

		/// <summary>
		/// ルートシグネチャの作成
		/// </summary>
		/// <returns>true:成功</returns>
		bool CreateRootSignature(ID3D12Device* device);

		/// <summary>
		/// 描画パイプラインの作成
		/// </summary>
		/// <returns></returns>
		bool CreatePipeline(ID3D12Device* device);

	public:

		LinePipeline();
		virtual ~LinePipeline() = default;

		/// <summary>
		/// 作成
		/// </summary>
		/// <returns>true:成功</returns>
		bool Initialize();

		// アクセサ
		ID3D12RootSignature* GetRootSignature() const { return mRootSignature.Get(); }
		ID3D12PipelineState* GetPipelineState() const { return mPipelineState.Get(); }
		D3D_PRIMITIVE_TOPOLOGY GetTopology()    const { return D3D_PRIMITIVE_TOPOLOGY_LINELIST; }

		static constexpr UINT SLOT_CAMERA_BUFFER = 0; // b0

	private:
		RootSig mRootSignature;
		PSO mPipelineState;

	};
}


