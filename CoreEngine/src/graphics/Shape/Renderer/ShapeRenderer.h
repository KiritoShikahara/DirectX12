#pragma once

#include<memory>
#include<vector>
#include<d3d12.h>
#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<Utility/Singleton/Singleton.hpp>
#include<graphics/Color/Color.h>

#include<graphics/Shape/Pipeline/ShapePipeline.h>
#include<graphics/StructuredBuffer/StructuredBuffer.h>
#include<graphics/VertexBuffer/VertexBuffer.h>

namespace ecs
{
	struct Transform;
	struct Shape;
}

namespace sys
{
	class Window;
}

namespace graphics
{
	class DX12Device;
	class DX12Context;
	class GDescriptorHeapManager;
	class ShaderManager;

	/// <summary>
	/// ShapeHeader.hlsli の ShapeShaderData と同一レイアウトの CPU 側ミラー
	/// </summary>
	struct ShapeShaderData
	{
		DirectX::XMFLOAT4X4 WVP;
		Color   Color = ::graphics::Color::White;
		float                Intensity = 1;
		float                FillAmount = 1;
		int                  ShapeType = 0;
		int                  FillType = 0;   
	};

	class ShapeRenderer : public utility::Singleton<ShapeRenderer>
	{
		SINGLETON_CLASS(ShapeRenderer);
	public:
		SINGLETON_ACCESSOR(ShapeRenderer);

		/// <summary>
		/// 初期化。依存するオブジェクトをすべて引数で受け取る。
		/// Shape はテクスチャを持たないため SpriteRenderer と異なり
		/// Texture 関連の依存は不要。
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

		/// <summary>リソース解放</summary>
		void Finalize();

		/// <summary>前フレームの描画データをクリアする。</summary>
		void Begin();

		/// <summary>
		/// 図形1件を描画予約する。
		/// テクスチャを持たないため、同種のバッチ化はインスタンス配列への
		/// 単純な追記のみで行う（全件を1ドローコールでまとめて発行）。
		/// </summary>
		void Draw(const ShapeShaderData& data);

		/// <summary>
		/// 予約済みデータを GPU コマンドとして発行する。
		/// DX12Renderer::Flip() より前に呼ぶこと。
		/// </summary>
		void End(ID3D12GraphicsCommandList* cmdList);

		/// <summary>
		/// ECS レジストリから Transform + Shape を持つエンティティを収集し、
		/// レイヤー順にソートして Draw() を発行する。
		/// </summary>
		void UpdateAndDraw(entt::registry& registry);
	private:
		/// <summary>
		/// Transform と Shape からシェーダーに渡すデータを計算する。
		/// </summary>
		ShapeShaderData CalculateShaderData(
			const ecs::Transform& tr,
			const ecs::Shape& sp) const;

		// 図形描画予約最大数
		static constexpr uint32_t MAX_SHAPE_COUNT = 4096;

		// UpdateAndDraw() 内で収集する描画対象1件分
		struct RenderItem
		{
			const ecs::Transform* Transform = nullptr;
			const ecs::Shape*     Shape = nullptr;
		};

		// GPU オブジェクト
		std::unique_ptr<ShapePipeline>     mPipeline;
		std::unique_ptr<StructuredBuffer>  mInstanceBuffer;
		std::unique_ptr<VertexBuffer>      mVB;

		// フレームデータ
		std::vector<ShapeShaderData>       mReservedData;

		// UpdateAndDraw()の一時バッファ。毎フレームclear()して再利用する
		std::vector<RenderItem>            mRenderItems;

		// 依存オブジェクト
		GDescriptorHeapManager* mHeapManager = nullptr;
		sys::Window* mWindow = nullptr;

	};

}