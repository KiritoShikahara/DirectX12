#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct RicochetWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.0f;     // 発射間隔(秒)
        float       ProjectileSpeed = 60.0f; // 弾速 m/s
        float       Damage = 6.0f;           // 火力
        float       Radius = 1.0f;           // 半径(m)
        float       HitRadiusMultiplier = 1.5f; // 判定拡大倍率
        float       ProjectileLifeTime = 5.0f; // 寿命(秒)
        float       SearchRadius = 60.0f;    // 索敵範囲(m)
        int         SplitCount = 2;          // 分裂子弾数
        int         MaxGeneration = 5;       // 最大増殖世代
        float       SplitSearchRadius = 30.0f; // 分裂索敵範囲(m)
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        std::string HitEffectIds;            // ヒットエフェクト素材ID

        REFLECT_BEGIN(RicochetWeaponData, "ricochet_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(ProjectileSpeed)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(Radius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ProjectileLifeTime)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_INT(SplitCount)
            REFLECT_FIELD_INT(MaxGeneration)
            REFLECT_FIELD_FLOAT(SplitSearchRadius)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(HitEffectIds)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::RicochetWeaponData);