#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Nova型武器のマスタデータ（CSV/DB）。
    /// WeaponComponent::WeaponID と対応する。
    /// ダメージ・範囲半径は Base + PerLevel * (Level - 1) の線形成長とする
    /// （他のWeaponDataと同じ方式）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、PulseInterval秒ごとにプレイヤー自身を中心とした円形範囲へ
    /// 即座にダメージを与える（NovaWeaponSystemが担当）。狙い・移動を必要としない
    /// 完全自動の持続武器（パーク選択でのみ取得可能）。
    /// </summary>
    struct NovaWeaponData
    {
        int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
        std::string Name;                    // 表示・デバッグ用

        float       PulseInterval = 1.5f;    // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       BaseDamage = 10.0f;      // Lv1火力
        float       DamagePerLevel = 3.0f;   // レベル毎の火力増加量

        float       BaseRadius = 15.0f;      // Lv1のエフェクト見た目基準半径(m)
        float       RadiusPerLevel = 2.0f;   // レベル毎の見た目基準半径増加量

        float       HitRadiusMultiplier = 2.0f; // 実際の当たり判定半径 = 上記(BaseRadius系)×この値。
                                                 // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       HeightOffset = 30.0f;    // 発生位置のY座標 = Owner.Position.y + この値(m)

        std::string EffectPath;              // 発動時エフェクト(.efk)

        REFLECT_BEGIN(NovaWeaponData, "nova_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(PulseInterval)
            REFLECT_FIELD_FLOAT(BaseDamage)
            REFLECT_FIELD_FLOAT(DamagePerLevel)
            REFLECT_FIELD_FLOAT(BaseRadius)
            REFLECT_FIELD_FLOAT(RadiusPerLevel)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(EffectPath)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::NovaWeaponData);
