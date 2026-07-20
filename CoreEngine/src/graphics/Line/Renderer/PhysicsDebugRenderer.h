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
	/// 当たり判定のデバック用のワイヤーフレームを表示する。
	///
	/// 他のレンダラーと同じく Begin() → UpdateAndDraw() → End() の3段構成を取る。
	///   Begin()         : 前フレームのデータをクリアする
	///   UpdateAndDraw() : registry / Jolt から頂点を収集し GPU バッファへ転送する（収集フェーズ）
	///   End()           : コマンドリストへの記録のみを行う（記録フェーズ）
	///
	/// 記録フェーズでは registry にも GPU バッファの Update にも触れないため、
	/// 将来チャネルを別スレッドで記録しても安全。
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
		/// 前フレームの描画データをクリアする。
		/// 収集フェーズの先頭で呼ぶこと。
		/// </summary>
		void Begin();

		/// <summary>
		/// registry と Jolt からコライダー形状を収集し、
		/// カメラ定数バッファと頂点バッファへ転送する。
		/// 収集フェーズ（シングルスレッド）で呼ぶこと。
		/// IsEnabled() == false のとき何もしない。
		/// </summary>
		void UpdateAndDraw(entt::registry& registry);

		/// <summary>
		/// 収集済みデータを GPU コマンドとして発行する。
		/// registry には触れないため、記録フェーズで呼べる。
		/// </summary>
		void End(ID3D12GraphicsCommandList* cmdList);

		/// <summary>
		/// 「Show Colliders」がONかどうか。ヒット時のデバッグ可視化用エンティティ
		/// （DebugWireSphereComponent）の生成要否を、各武器システム側から
		/// 判定するために公開している（OFF中に生成しても描画されず無駄なため）。
		/// </summary>
		bool IsEnabled() const { return mEnabled; }

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

		/// <summary>
		/// ワイヤーフレーム球を構築する（XY/XZ/YZの3つの円で近似）。
		/// DebugWireSphereComponent の可視化用。
		/// </summary>
		void PushWireSphere(
			const DirectX::XMFLOAT3& center,
			float radius,
			const DirectX::XMFLOAT4& color);

	private:
		std::unique_ptr<graphics::LinePipeline> mPipeline;

		/// <summary>動的頂点バッファ（毎フレーム Update）</summary>
		std::unique_ptr<graphics::VertexBuffer> mVertexBuffer;
		static constexpr size_t kMaxVertices = 131072;

		std::unique_ptr<graphics::ConstantBuffer> mCameraBuffer;

		graphics::GDescriptorHeapManager* mHeapManager = nullptr;
		bool mIsInitialized = false;

		// 全RigidBody+ColliderのJolt三角形抽出とライン構築を毎フレーム行うため、
		// Releaseでは無効を既定にする（ImGuiトグル自体をReleaseから除外しているため、
		// ここでOFFにしておかないと常時有効のまま切り替える手段がなくなる）。
		// Debugでは開発中の当たり判定確認のため、従来通りデフォルトONのままにする
		// （実測ではこの可視化自体はfps低下の主要因ではなかったため、開発体験を優先する）。
#ifdef _DEBUG
		bool              mEnabled = true;
#else
		bool              mEnabled = false;
#endif
		DirectX::XMFLOAT4 mDynamicColor = { 1.f, 0.f, 0.f, 1.f }; // 動的（赤）
		DirectX::XMFLOAT4 mStaticColor = { 0.f, 1.f, 0.f, 1.f }; // 静的（緑）
		DirectX::XMFLOAT4 mKinematicColor = { 0.f, 0.5f, 1.f, 1.f }; // キネマティック（青）
		DirectX::XMFLOAT4 mSensorColor = { 1.f, 1.f,  0.f, 1.f }; // センサー（黄）

		/// <summary>収集フェーズで構築される頂点リスト</summary>
		std::vector<WireVertex> mLineVertices;

		/// <summary>
		/// 今フレームに描画する頂点数。
		/// UpdateAndDraw() が確定し、End() が参照する。
		/// 0 のとき End() は何もしない。
		/// </summary>
		UINT mDrawVertexCount = 0;
	};
}