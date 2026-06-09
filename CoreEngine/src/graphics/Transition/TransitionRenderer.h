#pragma once
#include<Utility/Singleton/Singleton.hpp>

#include<graphics/Dx12/Dx12Type.h>
#include<graphics/Color/Color.h>

namespace graphics
{
	class TransitionRenderer : public utility::Singleton<TransitionRenderer>
	{
		SINGLETON_CLASS(TransitionRenderer);
	public:
		SINGLETON_ACCESSOR(TransitionRenderer);

		/// <summary>
		/// Sig + POS 作成
		/// </summary>
		void Initialize(ID3D12Device* device);

		/// <summary>
		/// 描画
		/// </summary>
		/// <param name="cmdList">コマンドリスト</param>
		/// <param name="color">色</param>
		void Draw(ID3D12GraphicsCommandList* cmdList, const graphics::Color& color);

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

	private:
		void CreateRootSignature(ID3D12Device* device);
		void CreatePSO(ID3D12Device* device);

	private:
		RootSig  mRootSignature;
		PSO mPSO;

	};
}


