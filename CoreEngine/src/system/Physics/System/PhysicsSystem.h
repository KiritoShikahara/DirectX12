#pragma once

#include<entt/entt.hpp>
#include<Utility/Export/Export.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace sys
{
	class ENGINE_API PhysicsSystem
	{
	public:

        /// <summary>
         /// RigidBodyComponent + ColliderComponent が揃っているが
         /// まだ Jolt に登録されていない Body を一括生成・登録する。
         /// SensorTagComponent があれば isSensor=true で登録する。
         /// </summary>
        static void BuildPendingBodies(entt::registry& registry);

        /// <summary>
        /// RigidBodyComponent::MoveVelocity / HasMoveRequest を Jolt に反映する。
        /// Update() より前（SyncFromTransform の後）に呼ぶこと。
        /// Dynamic は SetLinearVelocity、Kinematic は MoveKinematic で移動させる。
        /// 適用後 HasMoveRequest は false にリセットされる。
        /// </summary>
        static void ApplyMoveVelocity(entt::registry& registry, float fixedDeltaTime);

        /// <summary>
        /// Jolt のシミュレーションを 1 ステップ進める。
        /// FixedUpdate フェーズで呼ぶこと（固定タイムステップ推奨）。
        /// </summary>
        static void Update(entt::registry& registry, float fixedDeltaTime);

        /// <summary>
        /// Jolt の結果（位置・回転）を Transform に書き戻す。
        /// Update() の直後に呼ぶこと。
        /// Static / Kinematic は処理をスキップする。
        /// </summary>
        static void SyncToTransform(entt::registry& registry);

        /// <summary>
        /// TransformDirtyTag を持つエンティティの Transform を Jolt 側に反映する。
        /// ゲームロジックによる強制移動（テレポート等）の後、Update() より前に呼ぶこと。
        /// 処理後に TransformDirtyTag を削除する。
        /// </summary>
        static void SyncFromTransform(entt::registry& registry);

        /// <summary>
        /// フレーム末尾に CollisionEnterEvent / SensorEnterEvent を全削除する。
        /// PostUpdate フェーズで呼ぶこと。
        /// </summary>
        static void ClearCollisionEvents(entt::registry& registry);

        /// <summary>
        /// RigidBodyComponent が削除されたエンティティの Body を Jolt から除去する。
        /// フレーム末尾で呼ぶこと。
        /// </summary>
        static void DestroyPendingBodies(entt::registry& registry);

    private:
        /// <summary>
        /// eColliderShape と ColliderComponent のパラメータから
        /// Jolt の ShapeSettings を生成する
        /// </summary>
        static JPH::ShapeRefC BuildShape(const ecs::ColliderComponent& collider);

        /// <summary>
        /// ecs::eMotionType を JPH::EMotionType に変換する
        /// </summary>
        static JPH::EMotionType ToJoltMotionType(ecs::eMotionType motionType);

        /// <summary>
        /// ecs::eMotionType から ObjectLayer を決定する
        /// </summary>
        static JPH::ObjectLayer ToObjectLayer(ecs::eMotionType motionType, bool isSensor);
	};
}


