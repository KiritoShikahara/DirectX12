#pragma once

#include<DirectXMath.h>

namespace ecs
{
    /// <summary>
    /// 敵がノックバック中であることを示すコンポーネント。
    /// EnemyChaseSystemはこのコンポーネントを持つ敵への追従移動(MoveVelocity上書き)を
    /// スキップし、代わりにEnemyKnockbackSystemが速度を書き込む
    /// （2つのシステムが同一フレームでMoveVelocityを取り合わないようにするため）。
    /// RemainingTimeが0以下になった時点でEnemyKnockbackSystemが自身を取り外す。
    /// </summary>
    struct EnemyKnockbackComponent
    {
        /// <summary>吹き飛ばし速度(m/s、XZ平面のみ・Yは常に0)</summary>
        DirectX::XMFLOAT3 Velocity = { 0.0f, 0.0f, 0.0f };

        /// <summary>残り時間(秒)。0以下でEnemyChaseSystemの追従へ制御を戻す</summary>
        float RemainingTime = 0.0f;
    };
}
