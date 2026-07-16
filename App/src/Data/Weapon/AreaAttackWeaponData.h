#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// AreaAttack型武器のマスタデータ（CSV/DB）。
    /// WeaponComponent::WeaponID と対応する。
    /// ダメージ・範囲半径は Base + PerLevel * (Level - 1) の線形成長とする
    /// （SingleShotWeaponDataと同じ方式）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動時、狙い方向の SearchRadius 内から敵を最大 MaxTargets 体まで探し、
    /// 各敵の座標へ個別に氷柱(ハザード)を落とす。各氷柱は Duration 秒間持続し、
    /// TickInterval 秒ごとに BaseRadius 範囲内へダメージを与え続ける
    /// （AreaAttackHazardComponent / AreaAttackHazardSystem が担当）。
    /// </summary>
    struct AreaAttackWeaponData
    {
        int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.0f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       BaseDamage = 8.0f;       // 氷柱1個・1tickあたりのLv1火力
        float       DamagePerLevel = 2.0f;   // レベル毎の火力増加量

        float       BaseRadius = 15.0f;              // 氷柱1個あたりのエフェクト見た目基準半径(m、Lv1)
        float       RadiusPerLevel = 1.5f;          // レベル毎の見た目基準半径増加量

        float       HitRadiusMultiplier = 2.0f; // 実際の当たり判定半径 = 上記(BaseRadius系)×この値。
                                                 // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       SearchRadius = 105.0f;   // 発動時に敵を探す範囲(m)
        int         MaxTargets = 5;          // 同時に対象にする敵の最大数

        float       Duration = 2.0f;         // 氷柱の持続時間(秒)
        float       TickInterval = 0.5f;     // ダメージを与える間隔(秒)

        float       ForwardOffset = 3.0f;    // 探索範囲の中心を、プレイヤーの向いている方向へどれだけ離すか(m)

        std::string EffectPath;              // 発生時エフェクト(.efk)

        REFLECT_BEGIN(AreaAttackWeaponData, "area_attack_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(BaseDamage)
            REFLECT_FIELD_FLOAT(DamagePerLevel)
            REFLECT_FIELD_FLOAT(BaseRadius)
            REFLECT_FIELD_FLOAT(RadiusPerLevel)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_INT(MaxTargets)
            REFLECT_FIELD_FLOAT(Duration)
            REFLECT_FIELD_FLOAT(TickInterval)
            REFLECT_FIELD_FLOAT(ForwardOffset)
            REFLECT_FIELD_STR(EffectPath)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::AreaAttackWeaponData);
