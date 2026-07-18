#pragma once

#include<DirectXMath.h>

namespace ecs
{
    /// <summary>
    /// カメラの位置・注視点を、CameraFollowOffsetComponentによる通常のプレイヤー追従ではなく
    /// 外部から指定した固定値で上書きしたい場合にカメラエンティティへ付与する
    /// (必殺技演出等、排他的なカメラ演出を行いたいSystemが使う)。
    /// CameraPlayerFollowSystemがこのコンポーネントの有無を見て分岐し、実際のTransform書き込みは
    /// 常にCameraPlayerFollowSystem側で行う(カメラのTransformを書き込む権限を1箇所に集約するため、
    /// 各Systemはこのコンポーネントへ「リクエスト」を書くだけにする)。
    /// 演出が終わったら必ずこのコンポーネントをremoveし、通常追従へ戻すこと。
    /// </summary>
    struct CameraOverrideComponent
    {
        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 LookAt = { 0.0f, 0.0f, 0.0f };
    };
}
