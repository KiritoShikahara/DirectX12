#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// SingleShot型武器のマスタデータ（CSV/DB）。
    /// WeaponComponent::WeaponID と対応する。
    /// ダメージ・爆発半径は Base + PerLevel * (Level - 1) の線形成長とする。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    /// </summary>
    struct SingleShotWeaponData
    {
        int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.0f;     // 発射間隔(秒)。CooldownRateで乗算短縮される
        float       ProjectileSpeed = 20.0f; // 弾速 m/s

        float       BaseDamage = 5.0f;       // Lv1火力
        float       DamagePerLevel = 2.0f;   // レベル毎の火力増加量

        float       BaseExplosionRadius = 15.0f;    // Lv1爆発エフェクトの見た目基準半径(m)。敵3体分程度のサイズを想定
        float       ExplosionRadiusPerLevel = 2.0f; // レベル毎の見た目基準半径増加量

        float       HitRadiusMultiplier = 2.0f; // 実際の当たり判定半径 = 上記(BaseExplosionRadius系)×この値。
                                                 // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       ProjectileLifeTime = 3.0f; // 何にも当たらなかった場合に消滅するまでの秒数

        float       HeightOffset = 30.0f;    // 発射位置のY座標 = Owner.Position.y + この値(m)。
                                              // デフォルトはプレイヤーの胸あたりの高さ
                                              // (プレイヤーのColliderComponent半径・y=30を目安とする)

        std::string ProjectileEffectPath;    // 飛翔中エフェクト(.efk)
        std::string ExplosionEffectPath;     // 着弾時の爆発エフェクト(.efk)

        REFLECT_BEGIN(SingleShotWeaponData, "single_shot_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(ProjectileSpeed)
            REFLECT_FIELD_FLOAT(BaseDamage)
            REFLECT_FIELD_FLOAT(DamagePerLevel)
            REFLECT_FIELD_FLOAT(BaseExplosionRadius)
            REFLECT_FIELD_FLOAT(ExplosionRadiusPerLevel)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ProjectileLifeTime)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(ProjectileEffectPath)
            REFLECT_FIELD_STR(ExplosionEffectPath)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::SingleShotWeaponData);
