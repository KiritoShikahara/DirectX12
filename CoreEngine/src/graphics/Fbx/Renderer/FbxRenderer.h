#pragma once

#include <memory>
#include <vector>
#include <d3d12.h>
#include <entt/entt.hpp>
#include<Utility/Singleton/Singleton.hpp>

#include <graphics/Fbx/Pipeline/FbxPipeline.h>
#include <graphics/StructuredBuffer/StructuredBuffer.h>
#include<graphics/Fbx/Resouce/FbxSource.h>
#include<graphics/Color/Color.h>

namespace sys { class Window; }

namespace graphics
{
	class DX12Device;
	class GDescriptorHeapManager;
	class ShaderManager;

	/// <summary>
	/// FBXモデルの描画を担当するクラス。
	/// </summary>
	class FbxRenderer : public utility::Singleton<FbxRenderer>
	{
		SINGLETON_CLASS(FbxRenderer);
	public:
		SINGLETON_ACCESSOR(FbxRenderer);

		bool Initialize(
			DX12Device& device,
			GDescriptorHeapManager& heapManager,
			ShaderManager& shaderManager);

		/// <summary>フレーム先頭でバッファをクリアする</summary>
		void Begin();

		/// <summary>ECS レジストリから FbxModel エンティティを収集し描画登録する</summary>
		void UpdateAndDraw(entt::registry& registry);

		/// <summary>蓄積したドローコールを GPU コマンドとして発行する</summary>
		void End(ID3D12GraphicsCommandList* cmdList);

	private:

		void Submit(
			const FbxResource& resource,
			const DirectX::XMFLOAT4X4& world,
			const graphics::Color& tint,
			float                                       intensity,
			const std::vector<DirectX::XMFLOAT4X4>* boneMatrices);  // nullptr = スキニングなし

		/// <summary>
		/// ドローコール単位 セクションとインスタンス単位
		/// </summary>
		struct DrawCall
		{
			const FbxResource* Resource = nullptr;
			uint32_t                    SectionIndex = 0;
			uint32_t                    InstanceIndex = 0;   // mInstanceData 内のインデックス
		};

		// 上限定数
		static constexpr uint32_t MAX_FBX_INSTANCES = 512u;
		static constexpr uint32_t MAX_TOTAL_BONES = 32768u;  // 全エンティティ合計（512×64 ≒ 2MB）

		// GPUオブジェクト
		std::unique_ptr<FbxPipeline>     mPipeline;
		std::unique_ptr<StructuredBuffer> mInstanceBuffer;  // FbxInstanceData
		std::unique_ptr<StructuredBuffer> mBoneBuffer;       // XMFLOAT4X4

		// フレームデータ begin() でクリアして UpdateAndDraw() で蓄積する。End() で GPU 転送して発行する。
		std::vector<FbxInstanceData>          mInstanceData;
		std::vector<DirectX::XMFLOAT4X4>     mBoneData;      // 全エンティティ分を連結
		std::vector<DrawCall>                 mDrawCalls;

		// 依存
		GDescriptorHeapManager* mHeapManager = nullptr;
	};
}


