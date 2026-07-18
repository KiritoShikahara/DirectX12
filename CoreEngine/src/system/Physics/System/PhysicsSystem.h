#pragma once

#include<entt/entt.hpp>
#include<Utility/Export/Export.h>
#include<DirectXMath.h>
#include<vector>
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
        /// フレーム末尾に CollisionEnterEvent / CollisionStayEvent / SensorEnterEvent / SensorStayEvent を全削除する。
        /// PostUpdate フェーズで呼ぶこと。
        /// </summary>
        static void ClearCollisionEvents(entt::registry& registry);

        /// <summary>
        /// RigidBodyComponent が削除されたエンティティの Body を Jolt から除去する。
        /// フレーム末尾で呼ぶこと。
        /// </summary>
        static void DestroyPendingBodies(entt::registry& registry);

        /// <summary>
        /// RigidBodyComponent::MoveVelocity の X/Z を毎フレーム末にクリアする。
        /// PostUpdate フェーズで呼ぶこと。
        /// </summary>
        static void ClearMoveVelocity(entt::registry& registry);

        /// <summary>
        /// レイが最初にヒットした Body を entt::entity として返す(エディタのクリック選択用)。
        /// Collider を持たないエンティティはヒットしない。
        /// </summary>
        /// <param name="rayOrigin">レイの始点(ワールド座標)</param>
        /// <param name="rayDirection">レイの方向(非正規化可、長さは無視して maxDistance を使う)</param>
        /// <param name="maxDistance">レイの最大到達距離</param>
        /// <param name="outEntity">ヒットしたエンティティ(戻り値 true のときのみ有効)</param>
        /// <param name="outHitPoint">ヒット位置(ワールド座標、戻り値 true のときのみ有効)</param>
        /// <returns>true:ヒットした</returns>
        static bool TryPickEntity(
            entt::registry& registry,
            const DirectX::XMFLOAT3& rayOrigin,
            const DirectX::XMFLOAT3& rayDirection,
            float maxDistance,
            entt::entity& outEntity,
            DirectX::XMFLOAT3& outHitPoint);

        /// <summary>
        /// 球形範囲と重なっている Body を全て entt::entity として収集する(範囲攻撃等で使用)。
        /// Collider を持たないエンティティは対象にならない。センサー/通常Bodyを問わず収集するため、
        /// 対象を絞りたい場合は呼び出し側でタグ等によるフィルタを行うこと。
        /// </summary>
        /// <param name="center">球の中心(ワールド座標)</param>
        /// <param name="radius">球の半径</param>
        /// <param name="outEntities">重なっているエンティティを追加する(呼び出し前にクリアしない)</param>
        static void OverlapSphere(
            entt::registry& registry,
            const DirectX::XMFLOAT3& center,
            float radius,
            std::vector<entt::entity>& outEntities);

        /// <summary>
        /// entt 側で RigidBodyComponent(またはエンティティごと)が破棄される直前に呼ばれる。
        /// Body が生成済みなら Jolt から確実に除去し、孤立 Body の発生を防ぐ。
        /// registry.on_destroy&lt;RigidBodyComponent&gt;() のコールバックとして
        /// PhysicsManager::Initialize() で接続される。
        /// </summary>
        static void OnRigidBodyComponentDestroyed(entt::registry& registry, entt::entity entity);

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
        /// ecs::eMotionType から ObjectLayer を決定する。
        /// disableSelfCollision(RigidBodyComponent::DisableSelfCollision)がtrueの場合、
        /// 同じフラグを持つ他のBody同士(=敵同士)を衝突させないPhysicsLayer::EnemyMovingを返す
        /// </summary>
        static JPH::ObjectLayer ToObjectLayer(ecs::eMotionType motionType, bool isSensor, bool disableSelfCollision);
	};
}


