#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// 敵の初期ステータス マスタ（CSV / DB）。
    /// EnemyStatusComponent の Base（MaxHp / MoveSpeed / AtkPower）へ対応する。
    /// CSV ヘッダ名は各フィールド名と完全一致させること。
    /// </summary>
    struct EnemyData
    {
        int         Id = 0;     // 敵種ID（主キー。EnemyStatusComponent::EnemyId と対応）
        std::string Name;              // 表示名・デバッグ用
        float       MaxHp = 10.0f; // 最大HP
        float       MoveSpeed = 2.0f;  // 移動速度 m/s
        float       AtkPower = 1.0f;  // 接触ダメージ
        int         Exp = 1;     // 撃破時の獲得経験値

        REFLECT_BEGIN(EnemyData, "enemies")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(MaxHp)
            REFLECT_FIELD_FLOAT(MoveSpeed)
            REFLECT_FIELD_FLOAT(AtkPower)
            REFLECT_FIELD_INT(Exp)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::EnemyData);