#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include"../Pipeline/LinePipeline.h"
#include<graphics/VertexBuffer/VertexBuffer.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>
#include<graphics/ConstantBuffer/ConstantBuffer.h>

#include<memory>
#include<DirectXMath.h>
#include<vector>

namespace graphics
{
	class GDescriptorHeapManager;

	/// <summary>
	/// 当たり判定のデバック用のワイヤーフレームを表示する
	/// </summary>
	class PhysicsDebugRenderer : public utility::Singleton<PhysicsDebugRenderer>
	{
		SINGLETON_CLASS(PhysicsDebugRenderer);

	public:
		SINGLETON_ACCESSOR(PhysicsDebugRenderer);

		/// <summary>
		/// パイプライン・バッファを初期化する。
		/// DX12Renderer::Initialize() の後、アプリ起動時に一度だけ呼ぶ。
		/// ImGuiManager にデバッグウィンドウも登録する。
		/// </summary>
		bool Initialize();

		/// <summary>GPU リソースを解放する。アプリ終了時に呼ぶ。</summary>
		void Finalize();

		/// <summary>
		/// コライダーのワイヤーフレームを描画する。
		/// DX12Renderer::BeginFrame() の後・EndFrame() の前に呼ぶこと。
		/// IsEnabled() == false のとき即リターンする。
		/// </summary>
		void Draw(entt::registry& registry, ID3D12GraphicsCommandList* cmdList);

	private:

		/// <summary>ワイヤーフレーム頂点（Position + Color）</summary>
		struct WireVertex
		{
			DirectX::XMFLOAT3 Position;
			DirectX::XMFLOAT4 Color;
		};

		/// <summary>カメラ StructuredBuffer の要素型</summary>
		struct CameraData
		{
			DirectX::XMFLOAT4X4 ViewProjection; // CPU 側で転置済み
		};
	private:

		/// <summary>
		/// カメラのバッファ作成
		/// </summary>
		/// <returns></returns>
		bool CreateCameraBuffer();

		/// <summary>
		/// 設定変更用のウィンドウ追加
		/// </summary>
		void ImGuiWindow();

		/// <summary>
		/// ライン構築
		/// </summary>
		void PushLine(
			const DirectX::XMFLOAT3& from,
			const DirectX::XMFLOAT3& to,
			const DirectX::XMFLOAT4& color);

	private:
		std::unique_ptr<graphics::LinePipeline> mPipeline;

		/// <summary>動的頂点バッファ（毎フレーム Update）</summary>
		std::unique_ptr<graphics::VertexBuffer> mVertexBuffer;
		static constexpr size_t kMaxVertices = 131072;

		std::unique_ptr<graphics::ConstantBuffer> mCameraBuffer;

		graphics::GDescriptorHeapManager* mHeapManager = nullptr;
		bool mIsInitialized = false;

		bool              mEnabled = true;
		DirectX::XMFLOAT4 mDynamicColor = { 1.f, 0.f, 0.f, 1.f }; // 動的（赤）
		DirectX::XMFLOAT4 mStaticColor = { 0.f, 1.f, 0.f, 1.f }; // 静的（緑）
		DirectX::XMFLOAT4 mKinematicColor = { 0.f, 0.5f, 1.f, 1.f }; // キネマティック（青）
		DirectX::XMFLOAT4 mSensorColor = { 1.f, 1.f,  0.f, 1.f }; // センサー（黄）

		std::vector<WireVertex> mLineVertices;
	};
}


