#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct HomingMissileWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 0.6f;     // 発射間隔(秒)
        float       ProjectileSpeed = 55.0f; // 弾速 m/s
        float       TurnSpeed = 320.0f;      // 旋回速度(度/秒)
        float       Damage = 6.0f;           // 火力
        float       ExplosionRadius = 10.0f; // 爆発半径(m)
        float       HitRadiusMultiplier = 2.0f; // 判定拡大倍率
        float       ProjectileLifeTime = 4.0f; // 寿命(秒)
        float       SearchRadius = 80.0f;    // 索敵範囲(m)
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        std::string ProjectileEffectIds;     // 飛翔中エフェクト素材ID
        std::string ExplosionEffectIds;      // 爆発エフェクト素材ID

        REFLECT_BEGIN(HomingMissileWeaponData, "homing_missile_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(ProjectileSpeed)
            REFLECT_FIELD_FLOAT(TurnSpeed)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(ExplosionRadius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ProjectileLifeTime)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(ProjectileEffectIds)
            REFLECT_FIELD_STR(ExplosionEffectIds)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::HomingMissileWeaponData);