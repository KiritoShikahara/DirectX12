#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct SingleShotWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.0f;     // 発射間隔(秒)
        float       ProjectileSpeed = 20.0f; // 弾速 m/s
        float       Damage = 5.0f;           // 火力
        float       ExplosionRadius = 15.0f; // 爆発半径(m)
        float       HitRadiusMultiplier = 2.0f; // 判定拡大倍率
        float       ProjectileLifeTime = 3.0f; // 寿命(秒)
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        bool        ExplosionAtGroundLevel = false; // 地面着弾フラグ
        std::string ProjectileEffectIds;     // 飛翔中エフェクト素材ID
        std::string ExplosionEffectIds;      // 爆発エフェクト素材ID

        REFLECT_BEGIN(SingleShotWeaponData, "single_shot_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(ProjectileSpeed)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(ExplosionRadius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ProjectileLifeTime)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_BOOL(ExplosionAtGroundLevel)
            REFLECT_FIELD_STR(ProjectileEffectIds)
            REFLECT_FIELD_STR(ExplosionEffectIds)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::SingleShotWeaponData);