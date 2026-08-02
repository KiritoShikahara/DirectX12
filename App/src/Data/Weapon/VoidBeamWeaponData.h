#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct VoidBeamWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.8f;     // 発動間隔(秒)
        float       Damage = 7.0f;           // 火力
        float       SearchRadius = 90.0f;    // 索敵範囲(m)
        float       BeamLength = 90.0f;      // ビーム長さ(m)
        float       BeamWidth = 6.0f;        // ビーム幅(m)
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        std::string HitEffectIds;            // ヒットエフェクト素材ID
        float       HitEffectScale = 1.0f;   // ヒットエフェクト倍率
        int         MaxHitEffects = 5;       // 最大ヒットエフェクト再生数

        REFLECT_BEGIN(VoidBeamWeaponData, "void_beam_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_FLOAT(BeamLength)
            REFLECT_FIELD_FLOAT(BeamWidth)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(HitEffectIds)
            REFLECT_FIELD_FLOAT(HitEffectScale)
            REFLECT_FIELD_INT(MaxHitEffects)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::VoidBeamWeaponData);