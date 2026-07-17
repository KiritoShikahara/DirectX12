#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Cleave型武器のマスタデータ（CSV/DB）。
    /// WeaponComponent::WeaponID と対応する。
    /// ダメージ・射程は Base + PerLevel * (Level - 1) の線形成長とする
    /// （他のWeaponDataと同じ方式）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、FireInterval秒ごとに狙い方向(PlayerAimComponent::Direction)を
    /// 中心としたConeAngleDegrees(半角)の扇状範囲内にいる敵全員へ、即座に近接ダメージと
    /// ノックバックを与える完全自動の武器（CleaveWeaponSystemが担当）。
    /// ノックバックはEnemyKnockbackComponent/EnemyKnockbackSystemが処理する。
    /// </summary>
    struct CleaveWeaponData
    {
        int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       BaseDamage = 12.0f;      // Lv1火力
        float       DamagePerLevel = 3.0f;   // レベル毎の火力増加量

        float       BaseRadius = 20.0f;      // Lv1の扇状射程(m)
        float       RadiusPerLevel = 2.0f;   // レベル毎の射程増加量

        float       HitRadiusMultiplier = 1.2f; // 実際の判定射程 = 上記(BaseRadius系)×この値。
                                                  // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       ConeAngleDegrees = 60.0f; // 狙い方向からの扇状半角(度)。合計はこの2倍が有効角度

        float       KnockbackForce = 30.0f;   // ノックバックの初速(m/s)
        float       KnockbackDuration = 0.25f;// ノックバックが持続する時間(秒)

        float       HeightOffset = 30.0f;    // エフェクトの再生高さ = Owner.Position.y + この値(m)

        std::string EffectPath;              // 発動時エフェクト(.efk)

        REFLECT_BEGIN(CleaveWeaponData, "cleave_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(BaseDamage)
            REFLECT_FIELD_FLOAT(DamagePerLevel)
            REFLECT_FIELD_FLOAT(BaseRadius)
            REFLECT_FIELD_FLOAT(RadiusPerLevel)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ConeAngleDegrees)
            REFLECT_FIELD_FLOAT(KnockbackForce)
            REFLECT_FIELD_FLOAT(KnockbackDuration)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(EffectPath)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::CleaveWeaponData);
