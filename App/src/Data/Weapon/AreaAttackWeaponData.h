#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// AreaAttack型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動時、狙い方向の SearchRadius 内から敵を最大 MaxTargets 体まで探し、
    /// 各敵の座標へ個別に氷柱(ハザード)を落とす。各氷柱は Duration 秒間持続し、
    /// TickInterval 秒ごとに Radius 範囲内へダメージを与え続ける
    /// （AreaAttackHazardComponent / AreaAttackHazardSystem が担当）。
    /// </summary>
    struct AreaAttackWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.0f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       Damage = 8.0f;           // 氷柱1個・1tickあたりのこのレベルでの火力

        float       Radius = 15.0f;          // 氷柱1個あたりのエフェクト見た目基準半径(m、このレベル)

        float       HitRadiusMultiplier = 2.0f; // 実際の当たり判定半径 = Radius×この値。
                                                 // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       SearchRadius = 105.0f;   // 発動時に敵を探す範囲(m)
        int         MaxTargets = 5;          // 同時に対象にする敵の最大数

        float       Duration = 2.0f;         // 氷柱の持続時間(秒)
        float       TickInterval = 0.5f;     // ダメージを与える間隔(秒)

        float       ForwardOffset = 3.0f;    // 探索範囲の中心を、プレイヤーの向いている方向へどれだけ離すか(m)

        // エフェクト素材ID(';'区切りで複数指定可、data::EffectAssetData参照)。
        // ecs::effectutil::ResolveEffectIds()でパス文字列へ解決してから使うこと。
        std::string EffectIds;               // 発生時エフェクト

        int         AutoStrikeCount = 1;     // 手動発動とは別に、自動で落雷する本数(このレベルでの値)。
                                              // AreaAttackAutoStrikeSystemが使う。ダメージ/半径等は
                                              // 手動発動と全く同じ値(Damage/Radius等)を流用する

        REFLECT_BEGIN(AreaAttackWeaponData, "area_attack_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(Radius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_INT(MaxTargets)
            REFLECT_FIELD_FLOAT(Duration)
            REFLECT_FIELD_FLOAT(TickInterval)
            REFLECT_FIELD_FLOAT(ForwardOffset)
            REFLECT_FIELD_STR(EffectIds)
            REFLECT_FIELD_INT(AutoStrikeCount)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::AreaAttackWeaponData);
