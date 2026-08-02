#pragma once

#include <DirectXMath.h>
#include <Utility/Export/Export.h>

namespace ecs
{
    /// <summary>
    /// ワイヤーフレーム球を表示するデバッグ用コンポーネント
    /// </summary>
    struct ENGINE_API DebugWireSphereComponent
    {
        /// <summary>球の半径</summary>
        float Radius = 1.0f;
        /// <summary>描画色</summary>
        DirectX::XMFLOAT4 Color = { 1.0f, 0.5f, 0.0f, 1.0f };
    };
}