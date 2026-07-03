#include "pch.h"
#include "PhysicsSystem.h"

#include"../Manager/PhysicsManager.h"

// Jolt
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>

// ECS
#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>

namespace sys
{

    // ヘルパー：DirectXMath <=> Jolt　変換
    namespace
    {
        inline JPH::Vec3 ToJolt(const DirectX::XMFLOAT3& v)
        {
            return JPH::Vec3(v.x, v.y, v.z);
        }
        inline JPH::Quat ToJoltQuat(const DirectX::XMFLOAT4& q)
        {
            return JPH::Quat(q.x, q.y, q.z, q.w);
        }
        inline DirectX::XMFLOAT3 FromJolt(const JPH::Vec3& v)
        {
            return { v.GetX(), v.GetY(), v.GetZ() };
        }
        inline DirectX::XMFLOAT4 FromJoltQuat(const JPH::Quat& q)
        {
            return { q.GetX(), q.GetY(), q.GetZ(), q.GetW() };
        }
    
    } // anonymous namespace

    /// <summary>
    /// eColliderShape と ColliderComponent のパラメータから
    /// Jolt の ShapeSettings を生成する
    /// </summary>
    JPH::ShapeRefC PhysicsSystem::BuildShape(const ecs::ColliderComponent& collider)
    {
        JPH::ShapeRefC baseShape;

        switch (collider.Shape)
        {
        case ecs::eColliderShape::Box:
            baseShape = new JPH::BoxShape(ToJolt(collider.HalfExtent));
            break;

        case ecs::eColliderShape::Sphere:
            baseShape = new JPH::SphereShape(collider.Radius);
            break;

        case ecs::eColliderShape::Capsule:
            // Jolt の CapsuleShape は原点が中心、HalfHeight は円柱部分の半高さ
            baseShape = new JPH::CapsuleShape(collider.HalfHeight, collider.Radius);
            break;

        default:
            JPH_ASSERT(false, "Unknown collider shape");
            baseShape = new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
            break;
        }

        // オフセットがある場合は OffsetCenterOfMassShape でラップする
        const auto& offset = collider.Offset;
        if (offset.x != 0.f || offset.y != 0.f || offset.z != 0.f)
        {
            return new JPH::OffsetCenterOfMassShape(baseShape, ToJolt(offset));
        }

        return baseShape;
    }

    /// <summary>
    /// ecs::eMotionType を JPH::EMotionType に変換する
    /// </summary>
    JPH::EMotionType PhysicsSystem::ToJoltMotionType(ecs::eMotionType motionType)
    {
        switch (motionType)
        {
        case ecs::eMotionType::Static:    return JPH::EMotionType::Static;
        case ecs::eMotionType::Kinematic: return JPH::EMotionType::Kinematic;
        case ecs::eMotionType::Dynamic:   return JPH::EMotionType::Dynamic;
        default:                          return JPH::EMotionType::Dynamic;
        }
    }

    /// <summary>
    /// ecs::eMotionType から ObjectLayer を決定する
    /// </summary>
    JPH::ObjectLayer PhysicsSystem::ToObjectLayer(ecs::eMotionType motionType, bool isSensor)
    {
        if (isSensor)              return PhysicsLayer::Sensor;
        if (motionType == ecs::eMotionType::Static) return PhysicsLayer::NonMoving;
        return PhysicsLayer::Moving;
    }

    /// <summary>
    /// RigidBodyComponent + ColliderComponent が揃っているが
    /// まだ Jolt に登録されていない Body を一括生成・登録する。
    /// SensorTagComponent があれば isSensor=true で登録する。
    /// </summary>
    void PhysicsSystem::BuildPendingBodies(entt::registry& registry)
    {
        auto& bodyInterface = PhysicsManager::Get().GetBodyInterface();

        // RigidBodyComponent + ColliderComponent が揃っていて未生成の Body を対象にする
        registry.view<ecs::RigidBodyComponent, ecs::ColliderComponent, ecs::Transform>().each(
            [&](entt::entity entity,
                ecs::RigidBodyComponent& rb,
                const ecs::ColliderComponent& collider,
                const ecs::Transform& transform)
            {
                if (rb.IsBodyCreated)
                {
                    return; // すでに生成済み
                }

                // Shape 生成 
                JPH::ShapeRefC shape = BuildShape(collider);

                // センサーかどうか判定
                const bool isSensor = registry.all_of<ecs::SensorTagComponent>(entity);

                //  BodyCreationSettings 組み立て
                const auto& pos = transform.GetPosition();
                const auto& rot = transform.GetRotation();

                JPH::BodyCreationSettings settings(
                    shape,
                    ToJolt(pos),
                    ToJoltQuat(rot),
                    ToJoltMotionType(rb.MotionType),
                    ToObjectLayer(rb.MotionType, isSensor));

                settings.mFriction = rb.Friction;
                settings.mRestitution = rb.Restitution;
                settings.mIsSensor = isSensor;

                // Dynamic の場合は質量を設定
                if (rb.MotionType == ecs::eMotionType::Dynamic)
                {
                    settings.mOverrideMassProperties =
                        JPH::EOverrideMassProperties::CalculateInertia;
                    settings.mMassPropertiesOverride.mMass = rb.Mass;
                }

                // 回転拘束
                JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::All;
                if (rb.LockRotationX) dofs &= ~JPH::EAllowedDOFs::RotationX;
                if (rb.LockRotationY) dofs &= ~JPH::EAllowedDOFs::RotationY;
                if (rb.LockRotationZ) dofs &= ~JPH::EAllowedDOFs::RotationZ;
                settings.mAllowedDOFs = dofs;

                // Body 生成・アクティブ化
                JPH::Body* body = bodyInterface.CreateBody(settings);
                if (body == nullptr)
                {
                    // Body 数が上限に達している
                    JPH_ASSERT(false, "PhysicsSystem: Failed to create body (max body count reached)");
                    return;
                }

                bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);

                // UserData に entt::entity を書き込む（ContactListener が参照する）
                body->SetUserData(static_cast<JPH::uint64>(entt::to_integral(entity)));

                rb.BodyID = body->GetID();
                rb.IsBodyCreated = true;
            });
    }

    /// <summary>
    /// RigidBodyComponent::MoveVelocity / HasMoveRequest を Jolt に反映する。
    /// Update() より前（SyncFromTransform の後）に呼ぶこと。
    /// Dynamic は SetLinearVelocity、Kinematic は MoveKinematic で移動させる。
    /// 適用後 HasMoveRequest は false にリセットされる。
    /// </summary>
    void PhysicsSystem::ApplyMoveVelocity(entt::registry& registry, float fixedDeltaTime)
    {
        auto& bodyInterface = PhysicsManager::Get().GetBodyInterface();

        registry.view<ecs::RigidBodyComponent>().each(
            [&](ecs::RigidBodyComponent& rb)
            {
                if (!rb.IsBodyCreated)          return;
                if (!rb.HasMoveRequest)         return;
                if (rb.MotionType == ecs::eMotionType::Static) return;

                const JPH::Vec3 velocity = ToJolt(rb.MoveVelocity);

                if (rb.MotionType == ecs::eMotionType::Kinematic)
                {
                    // 現在位置・回転から目標位置を算出して MoveKinematic
                    JPH::Vec3 pos;
                    JPH::Quat rot;
                    bodyInterface.GetPositionAndRotation(rb.BodyID, pos, rot);

                    const JPH::Vec3 targetPos = pos + velocity * fixedDeltaTime;
                    bodyInterface.MoveKinematic(rb.BodyID, targetPos, rot, fixedDeltaTime);
                }
                else // Dynamic
                {
                    bodyInterface.SetLinearVelocity(rb.BodyID, velocity);
                    bodyInterface.ActivateBody(rb.BodyID);
                }

                rb.HasMoveRequest = false;
            });
    }

    /// <summary>
    /// Jolt のシミュレーションを 1 ステップ進める。
    /// FixedUpdate フェーズで呼ぶこと（固定タイムステップ推奨）。
    /// </summary>
    void PhysicsSystem::Update(entt::registry& /*registry*/, float fixedDeltaTime)
    {
        auto& mgr = PhysicsManager::Get();
        if (!mgr.IsInitialized())
        {
            return;
        }

        // Jolt の推奨する collision step 数（固定タイムステップで通常は 1）
        constexpr int collisionSteps = 1;

        mgr.GetPhysicsSystem().Update(
            fixedDeltaTime,
            collisionSteps,
            &mgr.GetTempAllocator(),
            &mgr.GetJobSystem()); // JobSystem は PhysicsSystem 内部に渡し済み
    }

    /// <summary>
    /// Jolt の結果（位置・回転）を Transform に書き戻す。
    /// Update() の直後に呼ぶこと。
    /// Static / Kinematic は処理をスキップする。
    /// </summary>
    void PhysicsSystem::SyncToTransform(entt::registry& registry)
    {
        const auto& bodyInterface = PhysicsManager::Get().GetBodyInterface();

        registry.view<ecs::RigidBodyComponent, ecs::Transform>().each(
            [&](ecs::RigidBodyComponent& rb, ecs::Transform& transform)
            {
                // 未生成・Static は書き戻し不要
                if (!rb.IsBodyCreated)                              return;
                if (rb.MotionType == ecs::eMotionType::Static)      return;

                JPH::Vec3 pos;
                JPH::Quat rot;
                bodyInterface.GetPositionAndRotation(rb.BodyID, pos, rot);

                transform.SetPosition(FromJolt(pos));
                transform.SetRotation(FromJoltQuat(rot));
                // ※ ここでは MarkDirty() が呼ばれるが、TransformDirtyTag は付けない
                //   （Jolt → Transform の同期なので再度 SyncFromTransform に回す必要はない）
            });
    }

    /// <summary>
    /// TransformDirtyTag を持つエンティティの Transform を Jolt 側に反映する。
    /// ゲームロジックによる強制移動（テレポート等）の後、Update() より前に呼ぶこと。
    /// 処理後に TransformDirtyTag を削除する。
    /// </summary>
    void PhysicsSystem::SyncFromTransform(entt::registry& registry)
    {
        auto& bodyInterface = PhysicsManager::Get().GetBodyInterface();

        // TransformDirtyTag が付いているエンティティのみ処理する
        auto view = registry.view<ecs::TransformDirtyTag, ecs::RigidBodyComponent, ecs::Transform>();
        view.each(
            [&](entt::entity entity,
                const ecs::RigidBodyComponent& rb,
                const ecs::Transform& transform)
            {
                if (!rb.IsBodyCreated)
                {
                    return;
                }

                const auto& pos = transform.GetPosition();
                const auto& rot = transform.GetRotation();

                bodyInterface.SetPositionAndRotation(
                    rb.BodyID,
                    ToJolt(pos),
                    ToJoltQuat(rot),
                    JPH::EActivation::Activate);

                // タグを削除（処理済みフラグとして機能）
                registry.erase<ecs::TransformDirtyTag>(entity);
            });
    }

    /// <summary>
    /// フレーム末尾に CollisionEnterEvent / SensorEnterEvent を全削除する。
    /// PostUpdate フェーズで呼ぶこと。
    /// </summary>
    void PhysicsSystem::ClearCollisionEvents(entt::registry& registry)
    {
        registry.clear<ecs::CollisionEnterEvent>();
        registry.clear<ecs::SensorEnterEvent>();


    }

    /// <summary>
    /// RigidBodyComponent が削除されたエンティティの Body を Jolt から除去する。
    /// フレーム末尾で呼ぶこと。
    /// </summary>
    void PhysicsSystem::DestroyPendingBodies(entt::registry& registry)
    {
        // この関数は entt の on_destroy シグナルと組み合わせるか、
        // または「削除待ちリスト」パターンで呼ぶ。
        // 現状は RigidBodyComponent が残っているが BodyId が無効なケースを掃除する。

        auto& bodyInterface = PhysicsManager::Get().GetBodyInterface();

        registry.view<ecs::RigidBodyComponent>().each(
            [&](ecs::RigidBodyComponent& rb)
            {
                // BodyId が有効なのに PhysicsSystem 側で消えているケースを安全に除去
                if (rb.IsBodyCreated && !bodyInterface.IsAdded(rb.BodyID))
                {
                    rb.IsBodyCreated = false;
                    rb.BodyID = JPH::BodyID();
                }
            });
    }

    void PhysicsSystem::ClearMoveVelocity(entt::registry& registry)
    {
        registry.view<ecs::RigidBodyComponent>().each(
            [&](ecs::RigidBodyComponent& rb)
            {
                rb.MoveVelocity.x = 0.f;
                rb.MoveVelocity.z = 0.f;
            });
    }

}