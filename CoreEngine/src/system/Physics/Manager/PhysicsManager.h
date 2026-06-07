#pragma once
#include<Utility/Singleton/Singleton.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Utility/Export/Export.h>
#include <entt/entt.hpp>

#include<cstdint>
#include<memory>

namespace sys
{
	/// <summary>
	/// オブジェクトのレイヤー定義
	/// </summary>
	namespace PhysicsLayer
	{
		static constexpr JPH::ObjectLayer NonMoving = 0; // 静的コライダー
		static constexpr JPH::ObjectLayer Moving = 1; // 動的・キネマティック
		static constexpr JPH::ObjectLayer Sensor = 2; // センサー（トリガー）
		static constexpr JPH::ObjectLayer Count = 3;
	}

	/// <summary>
	/// ブロードフェーズレイヤー定義
	/// AABBベースの粗めの衝突フェーズのグループ
	/// </summary>
	namespace BroadPhaseLayer
	{
		static constexpr JPH::BroadPhaseLayer NonMoving{ 0 };
		static constexpr JPH::BroadPhaseLayer Moving{ 1 };
		static constexpr JPH::uint            Count = 2;
	}


	class PhysicsManager : public utility::Singleton<PhysicsManager>
	{
	public:

		/// <summary>
		/// Jolt を初期化する。アプリ起動時に一度だけ呼ぶこと。
		/// </summary>
		/// <param name="registry">衝突イベントの書き込み先 EnTT レジストリ</param>
		/// <param name="maxBodies">同時に存在できる Body の最大数</param>
		/// <param name="maxBodyPairs">ブロードフェーズの最大ペア数</param>
		/// <param name="maxContactConstraints">コンタクト制約の最大数</param>
		/// <param name="tempAllocatorSizeMB">フレーム内一時アロケータのサイズ(MB)</param>
		/// <param name="numJobThreads">物理演算スレッド数（0=コア数-1）</param>
		bool Initialize(
			entt::registry& registry,
			uint32_t maxBodies = 4096,
			uint32_t maxBodyPairs = 4096,
			uint32_t maxContactConstraints = 2048,
			uint32_t tempAllocatorSizeMB = 10,
			uint32_t numJobThreads = 0);

		/// <summary>
		/// Jolt終了。アプリ終了時に一度呼ぶ。
		/// </summary>
		void Finalize();

		JPH::PhysicsSystem& GetPhysicsSystem() { return *mPhysicsSystem; }
		JPH::BodyInterface& GetBodyInterface() { return mPhysicsSystem->GetBodyInterface(); }
		JPH::TempAllocatorImpl& GetTempAllocator() { return *mTempAllocator; }
		bool                     IsInitialized() const { return mIsInitialized; }
	private:

		// 各フェーズ用のインナークラス
		class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
		{
		public:
			BroadPhaseLayerInterfaceImpl();
			uint32_t              GetNumBroadPhaseLayers() const override;
			JPH::BroadPhaseLayer  GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
			const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override;
#endif
		private:
			JPH::BroadPhaseLayer mObjectToBroadPhase[PhysicsLayer::Count];
		};

		class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
		{
		public:
			bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bpLayer) const override;
		};

		class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
		{
		public:
			bool ShouldCollide(JPH::ObjectLayer obj1, JPH::ObjectLayer obj2) const override;
		};


	private:
		bool mIsInitialized = false;

		// Jolt グローバル初期化は一度だけ
		static bool sJoltGlobalInitialized;

		// アロケーター・ジョブシステム
		std::unique_ptr<JPH::TempAllocatorImpl>    mTempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool>  mJobSystem;

		// レイヤーフィルター（PhysicsSystem より先に生存していること）
		BroadPhaseLayerInterfaceImpl        mBroadPhaseLayerInterface;
		ObjectVsBroadPhaseLayerFilterImpl   mObjectVsBroadPhaseLayerFilter;
		ObjectLayerPairFilterImpl           mObjectLayerPairFilter;

		// 物理システム本体
		std::unique_ptr<JPH::PhysicsSystem> mPhysicsSystem;

		// 衝突イベントリスナー
		std::unique_ptr<class ContactListener> mContactListener;

	};
}


