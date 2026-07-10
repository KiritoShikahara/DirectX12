#pragma once

#include<Utility/Export/Export.h>

namespace ecs
{
    /// <summary>
    /// 敵個体ごとの接触攻撃クールタイム。
    /// 多段ヒット・連続ヒットを防ぐために用いる。
    /// 攻撃力自体は EnemyStatusComponent::Current.AtkPower を参照する（ここには持たない）。
    /// </summary>
    struct ENGINE_API EnemyAttackComponent
    {
        /// <summary>攻撃間隔（秒）。この間隔を空けないと再度ダメージを与えない。</summary>
        float AttackInterval = 0.5f;

        /// <summary>残りクールタイム（秒）。0以下で攻撃可能。システムが毎フレーム減算する。</summary>
        float CooldownTimer = 0.0f;
    };
}