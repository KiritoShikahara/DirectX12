#pragma once

#include<Utility/Singleton/Singleton.hpp>

#include<graphics/Model/ModelData.h>
#include<graphics/Model/Resouce/ModelResouce.h>
#include<graphics/Model/Pipeline/ModelPipeline.h>
#include<graphics/StructuredBuffer/StructuredBuffer.h>
#include<graphics/Color/Color.h>

#include<DirectXMath.h>
#include<vector>
#include<memory>


namespace graphics
{

	class DX12Device;
	class GDescriptorHeapManager;
	class ShaderManager;

	/// <summary>
	/// 3Dモデル描画フロント
	/// </summary>
	class ModelRenderer : public utility::Singleton<ModelRenderer>
	{
		SINGLETON_CLASS(ModelRenderer);
	public:
		SINGLETON_ACCESSOR(ModelRenderer);

		/// <summary>
		/// PIpelineの作成
		/// </summary>
		/// <returns></returns>
		bool Initialize(
			DX12Device& device,
			GDescriptorHeapManager& heapManager,
			ShaderManager& shaderManager);

		/// <summary>フレーム先頭でバッファをクリア</summary>
		void Begin();

		/// <summary>ECS レジストリから Model + Transform エンティティを収集して描画登録</summary>
		void UpdateAndDraw(entt::registry& registry);

		/// <summary>蓄積した DrawCall を GPU コマンドとして発行</summary>
		void End(ID3D12GraphicsCommandList* cmdList);

	private:
		void Submit(
			const ModelResource& resource,
			const DirectX::XMFLOAT4X4& world,
			const graphics::Color& tint,
			float                              intensity,
			const std::vector<DirectX::XMFLOAT4X4>* boneMatrices);

		// DrawCall単位
		struct DrawCall
		{
			const ModelResource* Resource = nullptr;
			uint32_t             SectionIndex = 0;
			uint32_t             InstanceIndex = 0;
		};

	private:
		// 上限定数
		static constexpr uint32_t MAX_MODEL_INSTANCES = 512u;
		static constexpr uint32_t MAX_TOTAL_BONES = 32768u;

		// GPUオブジェクト
		std::unique_ptr<ModelPipeline>    mPipeline;
		std::unique_ptr<StructuredBuffer> mInstanceBuffer;
		std::unique_ptr<StructuredBuffer> mBoneBuffer;
		std::unique_ptr<StructuredBuffer> mCameraBuffer;

		// フレームデータ
		std::vector<ModelInstanceData>        mInstanceData;
		std::vector<DirectX::XMFLOAT4X4>     mBoneData;
		std::vector<DrawCall>                 mDrawCalls;

		// 依存
		GDescriptorHeapManager* mHeapManager = nullptr;
	};
}


