#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct AreaAttackWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.0f;     // 発動間隔(秒)
        float       Damage = 8.0f;           // 火力
        float       Radius = 15.0f;          // 半径(m)
        float       HitRadiusMultiplier = 2.0f; // 判定拡大倍率
        float       SearchRadius = 105.0f;   // 索敵範囲(m)
        int         MaxTargets = 5;          // 最大対象数
        float       Duration = 2.0f;         // 持続時間(秒)
        float       TickInterval = 0.5f;     // ダメージ間隔(秒)
        float       ForwardOffset = 3.0f;    // 前方オフセット(m)
        std::string EffectIds;               // エフェクト素材ID
        int         AutoStrikeCount = 1;     // 自動発動本数

        REFLECT_BEGIN(AreaAttackWeaponData, "area_attack_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(Radius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_INT(MaxTargets)
            REFLECT_FIELD_FLOAT(Duration)
            REFLECT_FIELD_FLOAT(TickInterval)
            REFLECT_FIELD_FLOAT(ForwardOffset)
            REFLECT_FIELD_STR(EffectIds)
            REFLECT_FIELD_INT(AutoStrikeCount)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::AreaAttackWeaponData);