#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
    struct FlickerStrikeWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)
        float       Damage = 15.0f;          // 火力
        int         ChargeHitCount = 2;      // 追加ワープ攻撃回数
        int         MaxCharge = 5;           // チャージ上限初期値
        int         MaxChargePerLevel = 1;   // レベルごとの上限加算数
        int         InitialCharge = 3;       // 初期チャージ数
        float       InitialTargetMaxRange = 60.0f; // 初期索敵最大距離(m)
        float       InitialSearchWidth = 6.0f;     // 初期索敵半幅(m)
        float       WarpSearchRadius = 200.0f;     // ワープ索敵範囲(m)
        float       WarpInterval = 0.08f;          // ワープ間隔(秒)
        float       TeleportOffset = 3.0f;         // テレポートオフセット(m)
        float       HeightOffset = 30.0f;    // 高さオフセット(m)
        std::string HitEffectIds;            // ヒットエフェクト素材ID
        float       HitEffectScale = 7.0f;   // ヒットエフェクト倍率

        REFLECT_BEGIN(FlickerStrikeWeaponData, "flicker_strike_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_INT(ChargeHitCount)
            REFLECT_FIELD_INT(MaxCharge)
            REFLECT_FIELD_INT(MaxChargePerLevel)
            REFLECT_FIELD_INT(InitialCharge)
            REFLECT_FIELD_FLOAT(InitialTargetMaxRange)
            REFLECT_FIELD_FLOAT(InitialSearchWidth)
            REFLECT_FIELD_FLOAT(WarpSearchRadius)
            REFLECT_FIELD_FLOAT(WarpInterval)
            REFLECT_FIELD_FLOAT(TeleportOffset)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(HitEffectIds)
            REFLECT_FIELD_FLOAT(HitEffectScale)
            REFLECT_END()
    };

    constexpr int kFlickerStrikeGlobalConfigId = 1001;
}

REFLECT_REGISTER(data::FlickerStrikeWeaponData);