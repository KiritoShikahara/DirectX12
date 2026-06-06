#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
    /// <summary>
    /// コライダーの形状種別
    /// </summary>
    enum class eColliderShape
    {
        Box,
        Sphere,
        Capsule,
    };

    struct ENGINE_API ColliderComponent
    {
        // 共通
        eColliderShape Shape = eColliderShape::Box;

        // Box
		DirectX::XMFLOAT3 HalfExtent = { 0.5f, 0.5f, 0.5f };

        // Sphere 
		float Radius = 0.5f;
        
        // Capsule
		float HalfHeight = 1.0f;

        // オフセット
        DirectX::XMFLOAT3 Offset = { 0.0f, 0.0f, 0.0f };

        // ファクトリ関数
        static ColliderComponent MakeBox(DirectX::XMFLOAT3 halfExtent, DirectX::XMFLOAT3 offset = {})
        {
            ColliderComponent c;
            c.Shape = eColliderShape::Box;
            c.HalfExtent = halfExtent;
            c.Offset = offset;
            return c;
        }
        static ColliderComponent MakeSphere(float radius, DirectX::XMFLOAT3 offset = {})
        {
            ColliderComponent c;
            c.Shape = eColliderShape::Sphere;
            c.Radius = radius;
            c.Offset = offset;
            return c;
        }
        static ColliderComponent MakeCapsule(float radius, float halfHeight, DirectX::XMFLOAT3 offset = {})
        {
            ColliderComponent c;
            c.Shape = eColliderShape::Capsule;
            c.Radius = radius;
            c.HalfHeight = halfHeight;
            c.Offset = offset;
            return c;
        }
    };

    /// <summary>
	/// 衝突開始イベント。衝突が開始したフレームに発行される。
    /// </summary>
    struct ENGINE_API CollisionEnterEvent
    {
        std::vector<entt::entity> OtherEntities;
    };

    /// <summary>
	/// センサーへの侵入イベント。センサーに他のエンティティが侵入したフレームに発行される。
    /// </summary>
    struct ENGINE_API SensorEnterEvent
    {
        std::vector<entt::entity> Visitors;
    };
}