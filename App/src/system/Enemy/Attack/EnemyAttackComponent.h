#pragma once

#include<Utility/Export/Export.h>

namespace ecs
{
    ///<summary>
    ///敵とプレイヤーの接触攻撃クールタイム。多段ヒットの連続ヒットを防ぐために用いる。攻撃力自体はEnemyStatusComponent::Current.AtkPowerを参照する
    ///</summary>
    struct ENGINE_API EnemyAttackComponent
    {
        ///<summary>
        ///攻撃間隔、秒。この間隔を空けないと再度ダメージを与えない
        ///</summary>
        float AttackInterval = 0.5f;

        ///<summary>
        ///残りクールタイム、秒。以下で攻撃可能。システムが毎フレーム減算する
        ///</summary>
        float CooldownTimer = 0.0f;
    };
}
