#pragma once

#include<memory>
#include<vector>
#include<d3d12.h>
#include<entt/entt.hpp>
#include<Utility/Singleton/Singleton.hpp>
#include<graphics/Data/GraphicsData.h>

#include<graphics/Sprite/Pipeline/SpritePipeline.h>
#include<graphics/StructuredBuffer/StructuredBuffer.h>
#include<graphics/VertexBuffer/VertexBuffer.h>

namespace ecs
{
	struct Transform;
	struct Sprite;
}

namespace sys
{
	class Window;
}

namespace graphics
{
	class DX12Device;
	class DX12Renderer;
	class GDescriptorHeapManager;
	class ShaderManager;

	class SpriteRenderer : public utility::Singleton<SpriteRenderer>
	{
		SINGLETON_CLASS(SpriteRenderer);
	public:
		SINGLETON_ACCESSOR(SpriteRenderer);

		/// <summary>
		/// 初期化。依存するオブジェクトをすべて引数で受け取る。
		/// </summary>
		/// <param name="device">GPU デバイス（バッファ作成・PSO 作成）</param>
		/// <param name="heapManager">ディスクリプタヒープの供給元</param>
		/// <param name="shaderManager">シェーダーのコンパイル・キャッシュ管理</param>
		/// <param name="window">仮想解像度の取得元</param>
		/// <returns>true:成功</returns>
		bool Initialize(
			DX12Device& device,
			GDescriptorHeapManager& heapManager,
			ShaderManager& shaderManager,
			sys::Window& window);

		/// <summary>前フレームの描画データをクリアする。</summary>
		void Begin();

		/// <summary>
		/// スプライト1件を描画予約する。
		/// 同一テクスチャが連続する場合は自動バッチ化される。
		/// </summary>
		void Draw(const SpriteShaderData& data, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);

		/// <summary>
		/// 予約済みデータを GPU コマンドとして発行する。
		/// DX12Renderer::Flip() より前に呼ぶこと。
		/// </summary>
		void End(ID3D12GraphicsCommandList* cmdList);

		/// <summary>
		/// ECS レジストリから Transform + Sprite を持つエンティティを収集し、
		/// レイヤー順にソートして Draw() を発行する。
		/// </summary>
		void UpdateAndDraw(entt::registry& registry);
	private:
		/// <summary>
		/// Transform と Sprite からシェーダーに渡すデータを計算する。
		/// </summary>
		SpriteShaderData CalculateShaderData(
			const ecs::Transform& tr,
			const ecs::Sprite& sp) const;

		// スプライト描画予約最大数
		static constexpr uint32_t MAX_SPRITE_COUNT = 4096;

		//　バッチ単位のドローコール用データ
		struct DrawCall
		{
			D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = {};
			uint32_t                    instanceCount = 0;
			uint32_t                    startIndex = 0;
		};

		// GPU オブジェクト
		std::unique_ptr<SpritePipeline>    mPipeline;
		std::unique_ptr<StructuredBuffer>  mInstanceBuffer;
		std::unique_ptr<VertexBuffer>      mVB;

		// フレームデータ
		std::vector<SpriteShaderData>      mReservedData;
		std::vector<DrawCall>              mDrawCalls;

		// 依存オブジェクト
		GDescriptorHeapManager* mHeapManager = nullptr;
		sys::Window* mWindow = nullptr;

	};

}

