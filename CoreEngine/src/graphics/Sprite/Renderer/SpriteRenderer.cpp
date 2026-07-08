#include"pch.h"
#include "SpriteRenderer.h"

// グラフィックス関連
#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Dx12/Dx12Renderer.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<graphics/Shader/ShaderManager.h>
#include<system/Window/Window.h>
#include<graphics/Texture/Texture.h>

// コンポーネント
#include<ecs/component/sprite/SpriteComponent.h>
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
	bool SpriteRenderer::Initialize(
		DX12Device& device,
		GDescriptorHeapManager& heapManager,
		ShaderManager& shaderManager,
		sys::Window& window)
	{
		// pipeline
		mPipeline = std::make_unique<SpritePipeline>();
		if (!mPipeline->Create(device, shaderManager))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "SpriteRenderer: Failed to create pipeline.");
			return false;
		}

		// インスタンスデータ転送用
		mInstanceBuffer = std::make_unique<StructuredBuffer>();
		if (!mInstanceBuffer->Create(sizeof(SpriteShaderData), MAX_SPRITE_COUNT))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "SpriteRenderer: Failed to create instance buffer.");
			return false;
		}

		// スプライト共通単位四角形
		SpriteVertex vertices[] = {
			{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }, // 左上
			{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } }, // 右上
			{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } }, // 左下
			{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } }, // 右下
		};

		mVB = std::make_unique<VertexBuffer>();
		if (!mVB->CreateDynamic(sizeof(vertices), sizeof(SpriteVertex)))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "SpriteRenderer: Failed to create vertex buffer.");
			return false;
		}
		mVB->Update(vertices, sizeof(vertices));

		// 依存オブジェクトの保存
		mHeapManager = &heapManager;
		mWindow = &window;

		// 描画予約の事前確保
		mReservedData.reserve(MAX_SPRITE_COUNT);
		mDrawCalls.reserve(256);	// 適当な数で予約

		DEBUG_LOG(sys::eLogLevel::Log, "SpriteRenderer: Initialized successfully.");

		return true;
	}

	void SpriteRenderer::Finalize()
	{
		mInstanceBuffer.reset();
		mPipeline.reset();
		mVB.reset();
		mReservedData.clear();
		mDrawCalls.clear();
		mHeapManager = nullptr;
		mWindow = nullptr;
	}

	/// <summary>前フレームの描画データをクリアする。</summary>
	void SpriteRenderer::Begin()
	{
		mReservedData.clear();
		mDrawCalls.clear();
	}

	/// <summary>
	/// スプライト1件を描画予約する。
	/// 同一テクスチャが連続する場合は自動バッチ化される。
	/// </summary>
	void SpriteRenderer::Draw(const SpriteShaderData& data, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle)
	{
		if (mReservedData.size() >= MAX_SPRITE_COUNT) return;

		// 同じテクスチャハンドルが連続するならバッチにまとめる
		if (!mDrawCalls.empty() &&
			mDrawCalls.back().textureHandle.ptr == textureHandle.ptr)
		{
			++mDrawCalls.back().instanceCount;
		}
		else
		{
			mDrawCalls.push_back({
				textureHandle,
				1u,
				static_cast<uint32_t>(mReservedData.size())
				});
		}

		mReservedData.push_back(data);
	}

	/// <summary>
	/// 予約済みデータを GPU コマンドとして発行する。
	/// DX12Renderer::Flip() より前に呼ぶこと。
	/// </summary>
	void SpriteRenderer::End(ID3D12GraphicsCommandList* cmdList)
	{
		if (mReservedData.empty()) return;

		// StructuredBuffer に全インスタンスデータを一括転送
		mInstanceBuffer->Update(
			mReservedData.data(),
			sizeof(SpriteShaderData) * mReservedData.size());

		// PSO / RootSignature のセット
		cmdList->SetPipelineState(mPipeline->GetPipelineState());
		cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());

		// SRV ヒープのセット（SetDescriptorHeaps は Flip 前の最後呼び出しが有効）
		ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

		// 頂点バッファとトポロジーのセット
		mVB->Set(cmdList, 0);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// [t0] StructuredBuffer（全スプライトの定数データ）
		cmdList->SetGraphicsRootDescriptorTable(0, mInstanceBuffer->GetGpuHandle());

		// バッチごとに [t1] テクスチャを差し替え、[b0] にインスタンス先頭オフセットをセットして
		// DrawInstanced を発行する。
		// 注意: SV_InstanceID の StartInstanceLocation 加算は GPU/ドライバ依存で信頼できないため、
		// StartInstanceLocation には常に 0 を渡し、代わりに Root32BitConstant でオフセットを渡して
		// VS 側 (InstanceOffset + SV_InstanceID) で手動加算する。
		for (const auto& call : mDrawCalls)
		{
			cmdList->SetGraphicsRootDescriptorTable(1, call.textureHandle);
			cmdList->SetGraphicsRoot32BitConstant(
				SpritePipeline::INSTANCE_OFFSET_ROOT_PARAM_INDEX, call.startIndex, 0);
			cmdList->DrawInstanced(4, call.instanceCount, 0, 0);
		}
	}

	/// <summary>
	/// ECS レジストリから Transform + Sprite を持つエンティティを収集し、
	/// レイヤー順にソートして Draw() を発行する。
	/// </summary>
	void SpriteRenderer::UpdateAndDraw(entt::registry& registry)
	{
		// Transform + Sprite を持つエンティティを収集
		auto view = registry.view<ecs::Transform, ecs::Sprite>();

		struct RenderItem
		{
			const ecs::Transform* transform;
			const ecs::Sprite* sprite;
		};

		std::vector<RenderItem> items;
		items.reserve(view.size_hint());

		view.each([&](auto, ecs::Transform& tr, ecs::Sprite& sp)
			{
				if (!sp.IsVisible || !sp.Texture) return;
				items.push_back({ &tr, &sp });
			});

		// Layer 昇順でソート（値が小さいほど手前＝後から描く）
		std::sort(items.begin(), items.end(),
			[](const RenderItem& a, const RenderItem& b)
			{
				return a.sprite->Layer < b.sprite->Layer;
			});

		// ソート済み順で Draw を発行（バッチ化のためテクスチャ順が重要）
		for (const auto& item : items)
		{
			const SpriteShaderData shaderData =
				CalculateShaderData(*item.transform, *item.sprite);
			Draw(shaderData, item.sprite->Texture->GetGpuHandle());
		}
	}

	/// <summary>
	/// Transform と Sprite からシェーダーに渡すデータを計算する。
	/// </summary>
	SpriteShaderData SpriteRenderer::CalculateShaderData(const ecs::Transform& tr, const ecs::Sprite& sp) const
	{
		using namespace DirectX;

		// 仮想解像度（毎フレーム取得せず Window 参照で取得）
		const float vWidth = static_cast<float>(mWindow->GetVirtualWidth());
		const float vHeight = static_cast<float>(mWindow->GetVirtualHeight());

		// 描画サイズの決定（Size が 0 のときはテクスチャの実サイズを使用）
		const float baseW = (sp.Size.x > 0.0f) ? sp.Size.x : sp.Texture->GetWidth();
		const float baseH = (sp.Size.y > 0.0f) ? sp.Size.y : sp.Texture->GetHeight();

		// Transform の Scale を反映（DrawScale と別軸で乗算する）
		const XMFLOAT3& trScale = tr.GetScale();
		const float w = baseW * sp.DrawScale.x * trScale.x;
		const float h = baseH * sp.DrawScale.y * trScale.y;

		// 統合 Transform から 2D 位置・回転を取得
		const XMFLOAT2 pos2D = tr.Get2DPosition();
		const float    rotRad = tr.Get2DRotation();

		// ワールド行列の合成:
		//   Pivot（基準点オフセット） → Scale + Flip → Rotation(Z) → Translation
		const XMMATRIX mPivot = XMMatrixTranslation(-sp.Pivot.x, -sp.Pivot.y, 0.0f);
		const XMMATRIX mScale = XMMatrixScaling(w * sp.Flip.x, h * sp.Flip.y, 1.0f);
		const XMMATRIX mRot = XMMatrixRotationZ(rotRad);
		const XMMATRIX mTrans = XMMatrixTranslation(pos2D.x, pos2D.y, 0.0f);
		const XMMATRIX world = mPivot * mScale * mRot * mTrans;

		// 正射影（Y 軸：上＝0、下＝Height）
		const XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
			0.0f, vWidth, vHeight, 0.0f, 0.0f, 1.0f);

		SpriteShaderData data;
		XMStoreFloat4x4(&data.WVP, XMMatrixTranspose(world * proj));
		data.Color = sp.Color;
		data.Intensity = sp.Intensity;
		data.FillAmount = sp.FillAmount;
		data.FillType = static_cast<int>(sp.FType);
		data.UVScale = sp.UVScale;
		data.UVOffset = sp.UVOffset;

		return data;
	}
}