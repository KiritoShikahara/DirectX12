#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    /// <summary>敵の基礎ステータスのマスタデータ(CSV/DB)。EnemyStatusComponent::Baseへ対応する</summary>
    struct EnemyData
    {
        int         Id = 0;            // 敵の種類ID(主キー。EnemyStatusComponent::EnemyIdと対応)
        std::string Name;              // 表示・デバッグ用
        float       MaxHp = 10.0f;     // 最大HP
        float       MoveSpeed = 2.0f;  // 移動速度 m/s
        float       AtkPower = 1.0f;   // 接触ダメージ
        int         Exp = 3;           // 撃破時の獲得経験値
        float       GoldValue = 3.0f;  // 撃破時に得られるゴールド(EnemyDeathSystem::AwardGold)

        REFLECT_BEGIN(EnemyData, "enemies")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(MaxHp)
            REFLECT_FIELD_FLOAT(MoveSpeed)
            REFLECT_FIELD_FLOAT(AtkPower)
            REFLECT_FIELD_INT(Exp)
            REFLECT_FIELD_FLOAT(GoldValue)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::EnemyData);