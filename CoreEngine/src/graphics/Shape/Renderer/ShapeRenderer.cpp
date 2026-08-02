#include"pch.h"
#include "ShapeRenderer.h"

// グラフィックス関連
#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Dx12/Dx12Renderer.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<graphics/Shader/ShaderManager.h>
#include<system/Window/Window.h>
#include<graphics/Data/GraphicsData.h>

// コンポーネント
#include<ecs/component/shape/ShapeComponent.h>
#include<ecs/component/transform/TransformComponent.h>

namespace graphics
{

	/// <summary>
	/// 初期化。依存するオブジェクトをすべて引数で受け取る。
	/// </summary>
	/// <param name="device">GPU デバイス（バッファ作成・PSO 作成）</param>
	/// <param name="heapManager">ディスクリプタヒープの供給元</param>
	/// <param name="shaderManager">シェーダーのコンパイル・キャッシュ管理</param>
	/// <param name="window">仮想解像度の取得元</param>
	/// <returns>true:成功</returns>
	bool ShapeRenderer::Initialize(
		DX12Device& device,
		GDescriptorHeapManager& heapManager,
		ShaderManager& shaderManager,
		sys::Window& window)
	{
		// pipeline
		mPipeline = std::make_unique<ShapePipeline>();
		if (!mPipeline->Create(device, shaderManager))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "ShapeRenderer: Failed to create pipeline.");
			return false;
		}

		// インスタンスデータ転送用
		mInstanceBuffer = std::make_unique<StructuredBuffer>();
		if (!mInstanceBuffer->Create(sizeof(ShapeShaderData), MAX_SHAPE_COUNT))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "ShapeRenderer: Failed to create instance buffer.");
			return false;
		}

		// 図形共通単位四角形（Sprite と同一の頂点レイアウト）
		SpriteVertex vertices[] = {
			{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }, // 左上
			{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } }, // 右上
			{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } }, // 左下
			{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } }, // 右下
		};

		//mVB = std::make_unique<VertexBuffer>();
		//if (!mVB->CreateDynamic(sizeof(vertices), sizeof(SpriteVertex)))
		//{
		//	DEBUG_LOG(sys::eLogLevel::Error, "ShapeRenderer: Failed to create vertex buffer.");
		//	return false;
		//}
		//mVB->Update(vertices, sizeof(vertices));

		mVB = std::make_unique<VertexBuffer>();
		if (!mVB->CreateStaticSync(vertices, sizeof(vertices), sizeof(SpriteVertex)))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "SpriteRenderer: Failed to create vertex buffer.");
			return false;
		}

		// 依存オブジェクトの保存
		mHeapManager = &heapManager;
		mWindow = &window;

		// 描画予約の事前確保
		mReservedData.reserve(MAX_SHAPE_COUNT);

		DEBUG_LOG(sys::eLogLevel::Log, "ShapeRenderer: Initialized successfully.");

		return true;
	}

	void ShapeRenderer::Finalize()
	{
		mInstanceBuffer.reset();
		mPipeline.reset();
		mVB.reset();
		mReservedData.clear();
		mHeapManager = nullptr;
		mWindow = nullptr;
	}

	/// <summary>前フレームの描画データをクリアする。</summary>
	void ShapeRenderer::Begin()
	{
		mReservedData.clear();
	}

	/// <summary>
	/// 図形1件を描画予約する。
	/// テクスチャを持たないため、Sprite のようなテクスチャ単位のバッチングは不要。
	/// 全件を単一の StructuredBuffer に積み、End() で 1 回の DrawInstanced にまとめる。
	/// </summary>
	void ShapeRenderer::Draw(const ShapeShaderData& data)
	{
		if (mReservedData.size() >= MAX_SHAPE_COUNT) return;

		mReservedData.push_back(data);
	}

	/// <summary>
	/// 予約済みデータを GPU コマンドとして発行する。
	/// DX12Renderer::Flip() より前に呼ぶこと。
	/// </summary>
	void ShapeRenderer::End(ID3D12GraphicsCommandList* cmdList)
	{
		if (mReservedData.empty()) return;

		// StructuredBuffer に全インスタンスデータを一括転送
		mInstanceBuffer->Update(
			mReservedData.data(),
			sizeof(ShapeShaderData) * mReservedData.size());

		// PSO / RootSignature のセット
		cmdList->SetPipelineState(mPipeline->GetPipelineState());
		cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());

		// SRV ヒープのセット
		ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

		// 頂点バッファとトポロジーのセット
		mVB->Set(cmdList, 0);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// 全図形の定数データ
		cmdList->SetGraphicsRootDescriptorTable(0, mInstanceBuffer->GetGpuHandle());

		// テクスチャの差し替えが無いため、全インスタンスを 1 回の DrawInstanced で発行
		cmdList->DrawInstanced(4, static_cast<uint32_t>(mReservedData.size()), 0, 0);
	}

	/// <summary>
	/// ECS レジストリから Transform + Shape を持つエンティティを収集し、
	/// レイヤー順にソートして Draw() を発行する。
	/// </summary>
	void ShapeRenderer::UpdateAndDraw(entt::registry& registry)
	{
		// 毎フレームのvector生成を避けるため、メンバ変数(mRenderItems)を使い回す
		mRenderItems.clear();
		auto view = registry.view<ecs::Transform, ecs::Shape>();
		mRenderItems.reserve(view.size_hint());

		view.each([&](auto, ecs::Transform& tr, ecs::Shape& sp)
			{
				if (!sp.IsVisible) return;
				mRenderItems.push_back({ &tr, &sp });
			});

		// レイヤーでソート
		std::sort(mRenderItems.begin(), mRenderItems.end(),
			[](const RenderItem& a, const RenderItem& b)
			{
				return a.Shape->Layer < b.Shape->Layer;
			});

		// ソート順で反映
		for (const auto& item : mRenderItems)
		{
			const ShapeShaderData shaderData =
				CalculateShaderData(*item.Transform, *item.Shape);
			Draw(shaderData);
		}
	}

	/// <summary>
	/// Transform と Shape からシェーダーに渡すデータを計算する。
	/// </summary>
	ShapeShaderData ShapeRenderer::CalculateShaderData(const ecs::Transform& tr, const ecs::Shape& sp) const
	{
		using namespace DirectX;

		// 仮想解像度
		const float vWidth = static_cast<float>(mWindow->GetVirtualWidth());
		const float vHeight = static_cast<float>(mWindow->GetVirtualHeight());

		// 描画サイズの決定
		const float w = sp.Size.x * sp.DrawScale.x;
		const float h = sp.Size.y * sp.DrawScale.y;

		// 統合 Transform から 2D 位置・回転を取得
		const XMFLOAT2 pos2D = tr.Get2DPosition();
		const float    rotRad = tr.Get2DRotation();

		// ワールド行列の合成
		const XMMATRIX mPivot = XMMatrixTranslation(-sp.Pivot.x, -sp.Pivot.y, 0.0f);
		const XMMATRIX mScale = XMMatrixScaling(w * sp.Flip.x, h * sp.Flip.y, 1.0f);
		const XMMATRIX mRot = XMMatrixRotationZ(rotRad);
		const XMMATRIX mTrans = XMMatrixTranslation(pos2D.x, pos2D.y, 0.0f);
		const XMMATRIX world = mPivot * mScale * mRot * mTrans;

		// 正射影（Y 軸：上＝0、下＝Height）
		const XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
			0.0f, vWidth, vHeight, 0.0f, 0.0f, 1.0f);

		ShapeShaderData data;
		XMStoreFloat4x4(&data.WVP, XMMatrixTranspose(world * proj));
		data.Color = sp.Color;
		data.Intensity = sp.Intensity;
		data.FillAmount = sp.FillAmount;
		data.ShapeType = static_cast<int>(sp.Type);
		data.FillType = static_cast<int>(sp.FType);

		return data;
	}
}