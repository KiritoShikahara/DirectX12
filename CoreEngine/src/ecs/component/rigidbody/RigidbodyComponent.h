#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Utility/Export/Export.h>
#include <DirectXMath.h>

namespace ecs
{
    /// <summary>
    /// RigidBody の動作モード。
    /// Jolt の EMotionType と対応させるが、エンジン側の型として定義する。
    /// </summary>
    enum class eMotionType
    {
        Static,    ///< 完全に静止
        Kinematic, ///< 物理演算に影響されず、手動で移動する
        Dynamic,   ///< 完全な物理演算対象
    };

    /// <summary>
    /// JoltPhysics の Body に対応するコンポーネント。
    /// </summary>
    struct ENGINE_API RigidBodyComponent
    {
        /// <summary>JoltのボディID</summary>
        JPH::BodyID BodyID;

        /// <summary>動作モード</summary>
        eMotionType MotionType = eMotionType::Dynamic;
        /// <summary>質量（kg）</summary>
        float Mass = 1.0f;
        /// <summary>摩擦係数 [0, 1]</summary>
        float Friction = 0.5f;
        /// <summary>反発係数 [0, 1]（0=反発なし、1=完全弾性）</summary>
        float Restitution = 0.0f;

        /// <summary>重力係数</summary>
        float GravityFactor = 1.0f;
        /// <summary>押し出しの減衰量</summary>
        float LinearDamping = 0.5f;

        /// <summary>X軸回転の固定フラグ</summary>
        bool LockRotationX = true;
        /// <summary>Y軸回転の固定フラグ</summary>
        bool LockRotationY = true;
        /// <summary>Z軸回転の固定フラグ</summary>
        bool LockRotationZ = true;

        /// <summary>
        /// 同じフラグを持つ他のDynamic Body同士の衝突を無効化するフラグ。
        /// </summary>
        bool DisableSelfCollision = false;

        /// <summary>ボディが作成済みかどうかのフラグ</summary>
        bool IsBodyCreated = false;

        /// <summary>Joltの回転をTransformに反映するかどうかのフラグ</summary>
        bool SyncRotation = false;

        /// <summary>入力等から加算された移動速度（m/s）</summary>
        DirectX::XMFLOAT3 MoveVelocity = { 0.0f, 0.0f, 0.0f };

        /// <summary>MoveVelocityをJoltに適用するリクエストフラグ</summary>
        bool HasMoveRequest = false;

        /// <summary>動的剛体を作成するファクトリメソッド</summary>
        static RigidBodyComponent MakeDynamic(float mass = 1.0f, float friction = 0.5f, float gravityFactor = 1.0f)
        {
            RigidBodyComponent rb;
            rb.MotionType = eMotionType::Dynamic;
            rb.Mass = mass;
            rb.Friction = friction;
            rb.GravityFactor = gravityFactor;
            return rb;
        }

        /// <summary>静的剛体を作成するファクトリメソッド</summary>
        static RigidBodyComponent MakeStatic(float friction = 0.5f)
        {
            RigidBodyComponent rb;
            rb.MotionType = eMotionType::Static;
            rb.Friction = friction;
            return rb;
        }

        /// <summary>キネマティック剛体を作成するファクトリメソッド</summary>
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