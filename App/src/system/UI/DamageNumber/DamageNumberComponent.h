#pragma once

#include<DirectXMath.h>

namespace ecs
{
    ///<summary>
    ///ダメージ数値の表示情報を保持
    ///</summary>
    struct DamageNumberComponent
    {
        ///<summary>
        ///現在のワールド座標
        ///</summary>
        DirectX::XMFLOAT3 WorldPosition = { 0.0f, 0.0f, 0.0f };

        ///<summary>
        ///上昇速度
        ///</summary>
        float RiseSpeed = 20.0f;

        ///<summary>
        ///残り表示時間
        ///</summary>
        float RemainingTime = 1.0f;

        ///<summary>
        ///表示時間
        ///</summary>
        float TotalTime = 1.0f;
    };
}