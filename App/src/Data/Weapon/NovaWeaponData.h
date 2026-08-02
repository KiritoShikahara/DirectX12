#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct NovaWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       PulseInterval = 1.5f;    // 発動間隔(秒)
        float       Damage = 10.0f;          // 火力
        float       Radius = 15.0f;          // 半径(m)
        float       HitRadiusMultiplier = 2.0f; // 判定拡大倍率
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        std::string EffectIds;               // 発動時エフェクト素材ID

        REFLECT_BEGIN(NovaWeaponData, "nova_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(PulseInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(Radius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(EffectIds)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::NovaWeaponData);