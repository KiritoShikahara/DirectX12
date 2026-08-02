#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct ChainLightningWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)
        float       Damage = 6.0f;           // 火力
        float       DamageFalloffPerJump = 0.75f; // ダメージ減衰率
        float       SearchRadius = 80.0f;    // 索敵範囲(m)
        float       JumpRadius = 25.0f;      // ジャンプ範囲(m)
        int         MaxJumps = 3;            // 最大ジャンプ回数
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        std::string HitEffectIds;            // ヒットエフェクト素材ID
        float       HitEffectScale = 4.0f;   // ヒットエフェクト倍率

        REFLECT_BEGIN(ChainLightningWeaponData, "chain_lightning_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(DamageFalloffPerJump)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_FLOAT(JumpRadius)
            REFLECT_FIELD_INT(MaxJumps)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(HitEffectIds)
            REFLECT_FIELD_FLOAT(HitEffectScale)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::ChainLightningWeaponData);