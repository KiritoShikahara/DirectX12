#pragma once

#include<DirectXMath.h>

namespace ecs
{
    ///<summary>
    ///敵がノックバック中であることを示すコンポーネント。EnemyChaseSystemは追従をスキップしEnemyKnockbackSystemが速度を書き込む。時間切れでEnemyKnockbackSystemが自身を取り外す
    ///</summary>
    struct EnemyKnockbackComponent
    {
        ///<summary>
        ///吹き飛ばし速度、m/s。XZ平面のみでYは常に0
        ///</summary>
        DirectX::XMFLOAT3 Velocity = { 0.0f, 0.0f, 0.0f };

        ///<summary>
        ///残り時間、秒。0以下でEnemyChaseSystemの追従へ制御を戻す
        ///</summary>
        float RemainingTime = 0.0f;
    };
}
