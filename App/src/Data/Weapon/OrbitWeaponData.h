#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct OrbitWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       Damage = 4.0f;           // 火力
        int         OrbCount = 2;            // オーブ数
        float       OrbitRadius = 9.0f;      // 周回半径(m)
        float       OrbitSpeed = 120.0f;     // 周回速度(度/秒)
        float       HitRadius = 3.0f;        // 判定半径(m)
        float       HitInterval = 0.5f;      // 再ダメージ間隔(秒)
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        float       ActiveDuration = 6.0f;   // アクティブ時間(秒)
        float       CooldownDuration = 3.0f; // クールタイム(秒)
        float       SlowMultiplier = 0.6f;   // 減速倍率
        float       SlowDuration = 1.5f;     // 減速持続時間(秒)
        std::string OrbEffectIds;            // オーブエフェクト素材ID
        std::string HitEffectIds;            // ヒットエフェクト素材ID

        REFLECT_BEGIN(OrbitWeaponData, "orbit_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_INT(OrbCount)
            REFLECT_FIELD_FLOAT(OrbitRadius)
            REFLECT_FIELD_FLOAT(OrbitSpeed)
            REFLECT_FIELD_FLOAT(HitRadius)
            REFLECT_FIELD_FLOAT(HitInterval)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_FLOAT(ActiveDuration)
            REFLECT_FIELD_FLOAT(CooldownDuration)
            REFLECT_FIELD_FLOAT(SlowMultiplier)
            REFLECT_FIELD_FLOAT(SlowDuration)
            REFLECT_FIELD_STR(OrbEffectIds)
            REFLECT_FIELD_STR(HitEffectIds)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::OrbitWeaponData);