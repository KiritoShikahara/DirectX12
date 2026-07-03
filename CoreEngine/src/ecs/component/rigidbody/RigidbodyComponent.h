#pragma once

#include<Jolt/Jolt.h>
#include<Jolt/Physics/Body/BodyID.h>
#include<Jolt/Physics/Body/MotionType.h>
#include<Utility/Export/Export.h>
#include<DirectXMath.h>

namespace ecs
{
    /// <summary>
    /// RigidBody の動作モード
    /// Jolt の EMotionType と対応させるが、エンジン側の型として定義する
    /// </summary>
    enum class eMotionType
    {
        Static,    // 完全に静止
        Kinematic, // 物理演算に影響されず、手動で移動する
        Dynamic,   // 完全な物理演算対象
    };

    /// <summary>
    /// JoltPhysics の Body に対応するコンポーネント。
    /// </summary>
    struct ENGINE_API RigidBodyComponent
    {
        // ID
		JPH::BodyID BodyID;

        // 物理設定
        eMotionType MotionType = eMotionType::Dynamic;
        float Mass = 1.0f; // 質量 kg
        float Friction = 0.5f; // 摩擦係数 [0, 1]
        float Restitution = 0.0f;        // 反発係数 [0, 1]（0=反発なし、1=完全弾性）

        // 拘束
        bool LockRotationX = false;
        bool LockRotationY = false;
        bool LockRotationZ = false;

        // 状態
        bool IsBodyCreated = false;

        // 入力等から加算された移動速度（m/s）。
        // PhysicsSystem::ApplyMoveVelocity() が消費して Jolt に反映する。
        DirectX::XMFLOAT3 MoveVelocity = { 0.0f, 0.0f, 0.0f };

        // true の間、MoveVelocity を毎フレーム Jolt に適用する。
        // 外部システムが移動量を積んだフレームで true にする。
        bool HasMoveRequest = false;

        // ファクトリ
        static RigidBodyComponent MakeDynamic(float mass = 1.0f, float friction = 0.5f)
        {
            RigidBodyComponent rb;
            rb.MotionType = eMotionType::Dynamic;
            rb.Mass = mass;
            rb.Friction = friction;
            return rb;
        }
        static RigidBodyComponent MakeStatic(float friction = 0.5f)
        {
            RigidBodyComponent rb;
            rb.MotionType = eMotionType::Static;
            rb.Friction = friction;
            return rb;
        }
        static RigidBodyComponent MakeKinematic(float friction = 0.5f)
        {
            RigidBodyComponent rb;
            rb.MotionType = eMotionType::Kinematic;
            rb.Friction = friction;
            return rb;
        }
    };

    /// <summary>
    /// Transform が外部から直接書き換えられたことを示すタグ。
    /// </summary>
    struct ENGINE_API TransformDirtyTag {};

}