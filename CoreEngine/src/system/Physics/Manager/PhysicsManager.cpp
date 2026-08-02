#include "pch.h"
#include "PhysicsManager.h"


// Jolt
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

#include "../System/PhysicsSystem.h"

using namespace JPH;
using namespace JPH::literals;



namespace sys
{
    static void TraceImpl(const char* inFMT, ...)
    {
        // メッセージ出力
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);

        // TTYに出力
        std::cout << buffer << std::endl;
    }

#ifdef JPH_ENABLE_ASSERTS

    // アサート時のコールバック
    static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
    {
        // TTYの出力
        std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
        return true;
    };
#endif

	bool PhysicsManager::sJoltGlobalInitialized = false;

    // 対応付け用
    PhysicsManager::BroadPhaseLayerInterfaceImpl::BroadPhaseLayerInterfaceImpl()
    {
        // ObjectLayer → BroadPhaseLayer のマッピング
        mObjectToBroadPhase[PhysicsLayer::NonMoving] = BroadPhaseLayer::NonMoving;
        mObjectToBroadPhase[PhysicsLayer::Moving] = BroadPhaseLayer::Moving;
        mObjectToBroadPhase[PhysicsLayer::Sensor] = BroadPhaseLayer::Moving; // センサーは Moving と同じ BP
        mObjectToBroadPhase[PhysicsLayer::EnemyMoving] = BroadPhaseLayer::Moving; // 敵も Moving と同じ BP
    }

    uint32_t PhysicsManager::BroadPhaseLayerInterfaceImpl::GetNumBroadPhaseLayers() const
    {
        return BroadPhaseLayer::Count;
    }

    JPH::BroadPhaseLayer PhysicsManager::BroadPhaseLayerInterfaceImpl::GetBroadPhaseLayer(
        JPH::ObjectLayer layer) const
    {
        JPH_ASSERT(layer < PhysicsLayer::Count);
        return mObjectToBroadPhase[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* PhysicsManager::BroadPhaseLayerInterfaceImpl::GetBroadPhaseLayerName(
        JPH::BroadPhaseLayer layer) const
    {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(layer))
        {
        case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayer::NonMoving): return "NonMoving";
        case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayer::Moving):    return "Moving";
        default: return "Unknown";
        }
    }
#endif

    /// ブロードフェーズのレイヤー判定
    bool PhysicsManager::ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(
        JPH::ObjectLayer layer, JPH::BroadPhaseLayer bpLayer) const
    {
        switch (layer)
        {
        case PhysicsLayer::NonMoving:
            // 静的オブジェクトは Moving とだけ衝突する
            return bpLayer == BroadPhaseLayer::Moving;
        case PhysicsLayer::Moving:
        case PhysicsLayer::EnemyMoving:
        case PhysicsLayer::Sensor:
            // 動的・敵・センサーは全てと衝突する(細かい除外はObjectLayerPairFilterImpl側で行う)
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }

    // 衝突レイヤー判定用
    bool PhysicsManager::ObjectLayerPairFilterImpl::ShouldCollide(
        JPH::ObjectLayer obj1, JPH::ObjectLayer obj2) const
    {
        switch (obj1)
        {
        case PhysicsLayer::NonMoving:
            return obj2 == PhysicsLayer::Moving || obj2 == PhysicsLayer::EnemyMoving || obj2 == PhysicsLayer::Sensor;
        case PhysicsLayer::Moving:
            return true; // Moving は全レイヤーと衝突
        case PhysicsLayer::EnemyMoving:
            // EnemyMoving同士だけは衝突させない。
            return obj2 != PhysicsLayer::EnemyMoving;
        case PhysicsLayer::Sensor:
            return obj2 == PhysicsLayer::Moving || obj2 == PhysicsLayer::EnemyMoving; // センサーは動的オブジェクト全般とのみ
        default:
            JPH_ASSERT(false);
            return false;
        }
    }

    /// <summary>
    /// Jolt を初期化する。アプリ起動時に一度だけ呼ぶこと。
    /// </summary>
    /// <param name="registry">衝突イベントの書き込み先 EnTT レジストリ</param>
    /// <param name="maxBodies">同時に存在できる Body の最大数</param>
    /// <param name="maxBodyPairs">ブロードフェーズの最大ペア数</param>
    /// <param name="maxContactConstraints">コンタクト制約の最大数</param>
    /// <param name="tempAllocatorSizeMB">フレーム内一時アロケータのサイズ(MB)</param>
    /// <param name="numJobThreads">物理演算スレッド数（0=コア数-1）</param>
    bool PhysicsManager::Initialize(entt::registry& registry, uint32_t maxBodies, uint32_t maxBodyPairs, uint32_t maxContactConstraints, uint32_t tempAllocatorSizeMB, uint32_t numJobThreads)
    {
        if (mIsInitialized)
        {
            return true;
        }

        // Joltグローバル初期化
        if (!sJoltGlobalInitialized)
        {
            if (JPH::Factory::sInstance == nullptr)
            {
                JPH::RegisterDefaultAllocator();

                Trace = TraceImpl;
                JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

                JPH::Factory::sInstance = new JPH::Factory();

#ifdef JPH_DOUBLE_PRECISION
#pragma message("JPH_DOUBLE_PRECISION ON")
#endif

#ifdef JPH_PROFILE_ENABLED
#pragma message("JPH_PROFILE_ENABLED ON")
#endif

#ifdef JPH_DEBUG_RENDERER
#pragma message("JPH_DEBUG_RENDERER ON")
#endif

#ifdef JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#pragma message("JPH_FLOATING_POINT_EXCEPTIONS_ENABLED ON")
#endif
                JPH::RegisterTypes();
            }
            sJoltGlobalInitialized = true;
        }

        // 一時アロケーター
        mTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(
            tempAllocatorSizeMB * 1024 * 1024);

        // ジョブシステム
        const uint32_t threads = (numJobThreads == 0)
            ? std::max(1u, std::thread::hardware_concurrency() - 1)
            : numJobThreads;

        mJobSystem = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            static_cast<int>(threads));

        // PhysicsSystem本体
        mPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
        mPhysicsSystem->Init(
            maxBodies,
            0,                               // numBodyMutexes（0=自動）
            maxBodyPairs,
            maxContactConstraints,
            mBroadPhaseLayerInterface,
            mObjectVsBroadPhaseLayerFilter,
            mObjectLayerPairFilter);

        mContactListener = std::make_unique<ContactListener>(registry);
        mPhysicsSystem->SetContactListener(mContactListener.get());

        // RigidBodyComponentが破棄される直前に
        // Jolt 側の Body を確実に除去する
        registry.on_destroy<ecs::RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyComponentDestroyed>();

        mIsInitialized = true;
        return true;
    }

    void PhysicsManager::Finalize()
    {
        if (!mIsInitialized)
        {
            return;
        }

        mPhysicsSystem.reset();
        mContactListener.reset();
        mJobSystem.reset();
        mTempAllocator.reset();

        // ファクトリの解放
        if (sJoltGlobalInitialized)
        {
            JPH::UnregisterTypes();
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
            sJoltGlobalInitialized = false;
        }

        mIsInitialized = false;
    }
}