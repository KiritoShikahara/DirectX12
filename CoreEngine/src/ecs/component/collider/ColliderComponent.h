#pragma once

#include <DirectXMath.h>
#include <Utility/Export/Export.h>
#include <entt/entt.hpp>
#include <vector>

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

    /// <summary>
    /// コライダーコンポーネント
    /// </summary>
    struct ENGINE_API ColliderComponent
    {
        /// <summary>コライダーの形状種別</summary>
        eColliderShape Shape = eColliderShape::Box;

        /// <summary>ボックスの半サイズ</summary>
        DirectX::XMFLOAT3 HalfExtent = { 0.5f, 0.5f, 0.5f };

        /// <summary>球の半径</summary>
        float Radius = 0.5f;

        /// <summary>カプセルの半高</summary>
        float HalfHeight = 1.0f;

        /// <summary>オフセット座標</summary>
        DirectX::XMFLOAT3 Offset = { 0.0f, 0.0f, 0.0f };

        /// <summary>ボックスコライダーを作成する</summary>
        static ColliderComponent MakeBox(DirectX::XMFLOAT3 halfExtent, DirectX::XMFLOAT3 offset = {})
        {
            ColliderComponent c;
            c.Shape = eColliderShape::Box;
            c.HalfExtent = halfExtent;
            c.Offset = offset;
            return c;
        }

        /// <summary>スフィアコライダーを作成する</summary>
        static ColliderComponent MakeSphere(float radius, DirectX::XMFLOAT3 offset = {})
        {
            ColliderComponent c;
            c.Shape = eColliderShape::Sphere;
            c.Radius = radius;
            c.Offset = offset;
            return c;
        }

        /// <summary>カプセルコライダーを作成する</summary>
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
    /// 衝突開始イベント
    /// </summary>
    struct ENGINE_API CollisionEnterEvent
    {
        /// <summary>衝突した相手のエンティティ一覧</summary>
        std::vector<entt::entity> OtherEntities;
    };

    /// <summary>
    /// 衝突継続イベント
    /// </summary>
    struct ENGINE_API CollisionStayEvent
    {
        /// <summary>衝突している相手のエンティティ一覧</summary>
        std::vector<entt::entity> OtherEntities;
    };

    /// <summary>
    /// センサーへの侵入イベント
    /// </summary>
    struct ENGINE_API SensorEnterEvent
    {
        /// <summary>侵入した訪問者のエンティティ一覧</summary>
        std::vector<entt::entity> Visitors;
    };

    /// <summary>
    /// センサー継続侵入イベント
    /// </summary>
    struct ENGINE_API SensorStayEvent
    {
        /// <summary>侵入している訪問者のエンティティ一覧</summary>
        std::vector<entt::entity> Visitors;
    };

    /// <summary>
    /// センサータグコンポーネント
    /// </summary>
    struct ENGINE_API SensorTagComponent {};
}