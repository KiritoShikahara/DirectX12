#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include"../Pipeline/LinePipeline.h"
#include<graphics/VertexBuffer/VertexBuffer.h>
#include<graphics/ConstantBuffer/ConstantBuffer.h>

#include<memory>
#include<DirectXMath.h>
#include<vector>
#include<entt/entt.hpp>

namespace graphics
{
	class GDescriptorHeapManager;

	/// <summary>
	/// DirectionalLightComponent の位置・方向を矢印(軸+矢じり)とマーカー(十字)で可視化する。
	///
	/// 他のデバッグレンダラー(PhysicsDebugRenderer)と同じく
	/// Begin() → UpdateAndDraw() → End() の3段構成を取り、LinePipeline を共用する。
	///   Begin()         : 前フレームのデータをクリアする
	///   UpdateAndDraw() : registry から矢印の頂点を構築し GPU バッファへ転送する（収集フェーズ）
	///   End()           : コマンドリストへの記録のみを行う（記録フェーズ）
	///
	/// 記録フェーズでは registry にも GPU バッファの Update にも触れないため、
	/// チャネルを別スレッドで記録しても安全。
	/// </summary>
	class LightDebugRenderer : public utility::Singleton<LightDebugRenderer>
	{
		SINGLETON_CLASS(LightDebugRenderer);

	public:
		SINGLETON_ACCESSOR(LightDebugRenderer);

		/// <summary>パイプライン・バッファを初期化する。アプリ起動時に一度だけ呼ぶ。</summary>
		bool Initialize();

		/// <summary>GPU リソースを解放する。アプリ終了時に呼ぶ。</summary>
		void Finalize();

		/// <summary>前フレームの描画データをクリアする。収集フェーズの先頭で呼ぶこと。</summary>
		void Begin();

		/// <summary>
		/// registry から DirectionalLightComponent を収集し、
		/// 矢印(方向)とマーカー(位置)の頂点をカメラ定数バッファと頂点バッファへ転送する。
		/// 収集フェーズ（シングルスレッド）で呼ぶこと。IsEnabled() == false のとき何もしない。
		/// </summary>
		void UpdateAndDraw(entt::registry& registry);

		/// <summary>
		/// 収集済みデータを GPU コマンドとして発行する。
		/// registry には触れないため、記録フェーズで呼べる。
		/// </summary>
		void End(ID3D12GraphicsCommandList* cmdList);

	private:
		/// <summary>ワイヤーフレーム頂点（Position + Color）</summary>
		struct WireVertex
		{
			DirectX::XMFLOAT3 Position;
			DirectX::XMFLOAT4 Color;
		};

		/// <summary>カメラ ConstantBuffer の要素型</summary>
		struct CameraData
		{
			DirectX::XMFLOAT4X4 ViewProjection; // CPU 側で転置済み
		};

	private:
		bool CreateCameraBuffer();
		void ImGuiWindow();

		/// <summary>ライン構築</summary>
		void PushLine(
			const DirectX::XMFLOAT3& from,
			const DirectX::XMFLOAT3& to,
			const DirectX::XMFLOAT4& color);

		/// <summary>
		/// 1つの Directional Light 分のギズモ(軸+矢じり+位置マーカー)を積む。
		/// lightPos: ShadowTarget - dir * ShadowDistance で求めたライト位置。
		/// </summary>
		void PushDirectionalLightGizmo(
			const DirectX::XMFLOAT3& lightPos,
			const DirectX::XMFLOAT3& target,
			const DirectX::XMFLOAT3& dir);

	private:
		std::unique_ptr<graphics::LinePipeline> mPipeline;

		/// <summary>動的頂点バッファ（毎フレーム Update）</summary>
		std::unique_ptr<graphics::VertexBuffer> mVertexBuffer;
		static constexpr size_t kMaxVertices = 4096;

		std::unique_ptr<graphics::ConstantBuffer> mCameraBuffer;

		graphics::GDescriptorHeapManager* mHeapManager = nullptr;
		bool mIsInitialized = false;

		bool              mEnabled = true;
		DirectX::XMFLOAT4 mGizmoColor = { 1.0f, 0.75f, 0.2f, 1.0f }; // 太陽光を連想させるオレンジ
		float             mMarkerSize = 1.0f; // 矢じり・位置マーカーのサイズ(ワールド単位)

		/// <summary>収集フェーズで構築される頂点リスト</summary>
		std::vector<WireVertex> mLineVertices;

		/// <summary>
		/// 今フレームに描画する頂点数。
		/// UpdateAndDraw() が確定し、End() が参照する。0 のとき End() は何もしない。
		/// </summary>
		UINT mDrawVertexCount = 0;
	};
}
