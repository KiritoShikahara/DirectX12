#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
    /// <summary>
    /// Transform の位置を中心にワイヤーフレーム球を表示するデバッグ用コンポーネント。
    /// PhysicsDebugRenderer(ImGui「Physics Debug」→「Show Colliders」)が対象にする。
    ///
    /// PhysicsDebugRenderer は Jolt に登録済みの Body(RigidBodyComponent)しか描画できないため、
    /// PhysicsSystem::OverlapSphere のようなアドホックな当たり判定クエリは何もしないと見えない。
    /// そうしたクエリの実際の判定範囲を可視化したい場合、このコンポーネントを一時エンティティに
    /// 付与する（Effekseerの見た目のエフェクトサイズとは独立して、実際のゲームロジック上の
    /// 判定半径を確認できる）。
    /// </summary>
    struct ENGINE_API DebugWireSphereComponent
    {
        float Radius = 1.0f;
        DirectX::XMFLOAT4 Color = { 1.0f, 0.5f, 0.0f, 1.0f }; // オレンジ(デフォルト)
    };
}
