#pragma once

#include<Utility/Export/Export.h>
#include<DirectXMath.h>

namespace ecs
{
    /// <summary>
    /// プレイヤーの攻撃方向（エイム方向）を保持するコンポーネント。
    /// </summary>
    struct ENGINE_API PlayerAimComponent
    {
        /// <summary>
        /// 攻撃方向（正規化済みのワールド空間ベクトル）。
        /// トップダウンのため Y は常に 0。
        /// </summary>
        DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };
    };
}