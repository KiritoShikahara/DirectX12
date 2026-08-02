#pragma once

#include<DirectXMath.h>

namespace ecs
{
    ///<summary>
    ///カメラの位置・注視点をCameraFollowOffsetComponentの通常追従ではなく外部指定の固定値で上書きしたい場合に付与する。CameraPlayerFollowSystemが唯一の書き込み元で、他Systemはリクエストを書くだけ。演出終了後は必ずremoveすること
    ///</summary>
    struct CameraOverrideComponent
    {
        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 LookAt = { 0.0f, 0.0f, 0.0f };
    };
}
