#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
    struct ENGINE_API CameraFollowOffsetComponent
    {
        ///<summary>
        ///カメラ位置のプレイヤー座標からのずれ量
        ///</summary>
        DirectX::XMFLOAT3 Offset = { 0.f, 5.f, -10.f };

        ///<summary>
        ///注視点のプレイヤー座標からのずれ量。頭の高さを見る想定
        ///</summary>
        DirectX::XMFLOAT3 LookAtOffset = { 0.f, 1.5f, 0.f };
    };
}
