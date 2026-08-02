#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct CleaveWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)
        float       Damage = 12.0f;          // 火力
        float       Radius = 20.0f;          // 扇状射程(m)
        float       HitRadiusMultiplier = 1.2f; // 判定拡大倍率
        float       ConeAngleDegrees = 60.0f; // 扇状半角(度)
        float       KnockbackForce = 30.0f;   // ノックバック初速(m/s)
        float       KnockbackDuration = 0.25f;// ノックバック持続時間(秒)
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        std::string EffectIds;               // 発動時エフェクト素材ID

        REFLECT_BEGIN(CleaveWeaponData, "cleave_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(Radius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ConeAngleDegrees)
            REFLECT_FIELD_FLOAT(KnockbackForce)
            REFLECT_FIELD_FLOAT(KnockbackDuration)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(EffectIds)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::CleaveWeaponData);