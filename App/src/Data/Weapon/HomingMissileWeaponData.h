#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Homing Missile型武器のマスタデータ（CSV/DB）。
    /// WeaponComponent::WeaponID と対応する。
    /// ダメージ・爆発半径は Base + PerLevel * (Level - 1) の線形成長とする
    /// （他のWeaponDataと同じ方式）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の最も近い敵へ自動で
    /// 追尾弾を発射する完全自動の武器（HomingMissileWeaponSystemが担当）。
    /// 発射後はProjectileComponent(IsHoming=true)としてHomingMissileSteeringSystemが
    /// 毎フレームTurnSpeedの範囲内で進行方向を対象へ向け続ける（対象を見失った場合は
    /// SearchRadius内で再検索する）。命中判定・爆発エフェクトは既存のProjectile汎用
    /// パイプライン(ProjectileMovementSystem/ProjectileCollisionSystem)をそのまま再利用する。
    /// </summary>
    struct HomingMissileWeaponData
    {
        int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 0.6f;     // 発射間隔(秒)。CooldownRateで乗算短縮される

        float       ProjectileSpeed = 55.0f; // 弾速 m/s
        float       TurnSpeed = 320.0f;      // 追尾時の最大旋回速度(度/秒)

        float       BaseDamage = 6.0f;       // Lv1火力（序盤の敵(MaxHp10)を2発で倒せる程度の値）
        float       DamagePerLevel = 1.5f;   // レベル毎の火力増加量

        float       BaseExplosionRadius = 10.0f;    // Lv1爆発エフェクトの見た目基準半径(m)
        float       ExplosionRadiusPerLevel = 1.5f; // レベル毎の見た目基準半径増加量

        float       HitRadiusMultiplier = 2.0f; // 実際の当たり判定半径 = 上記(BaseExplosionRadius系)×この値。
                                                 // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       ProjectileLifeTime = 4.0f; // 何にも当たらなかった場合に消滅するまでの秒数

        float       SearchRadius = 80.0f;    // 発射対象・見失った際の再捕捉対象を探す範囲(m)

        float       HeightOffset = 30.0f;    // 発射位置のY座標 = Owner.Position.y + この値(m)

        std::string ProjectileEffectPath;    // 飛翔中エフェクト(.efk)
        std::string ExplosionEffectPath;     // 着弾時の爆発エフェクト(.efk)

        REFLECT_BEGIN(HomingMissileWeaponData, "homing_missile_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(ProjectileSpeed)
            REFLECT_FIELD_FLOAT(TurnSpeed)
            REFLECT_FIELD_FLOAT(BaseDamage)
            REFLECT_FIELD_FLOAT(DamagePerLevel)
            REFLECT_FIELD_FLOAT(BaseExplosionRadius)
            REFLECT_FIELD_FLOAT(ExplosionRadiusPerLevel)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ProjectileLifeTime)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(ProjectileEffectPath)
            REFLECT_FIELD_STR(ExplosionEffectPath)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::HomingMissileWeaponData);
