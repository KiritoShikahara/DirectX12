#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Bone Spear型武器のマスタデータ（CSV/DB）。
    /// WeaponComponent::WeaponID と対応する。
    /// ダメージ・爆発半径は Base + PerLevel * (Level - 1) の線形成長とする
    /// （他のWeaponDataと同じ方式）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の最も近い敵へ向けて
    /// 直進する貫通弾を自動発射する完全自動の武器（BoneSpearWeaponSystemが担当）。
    /// Homing Missileと同じProjectile汎用パイプラインを流用するが、発射後は誘導せず
    /// (IsHoming=false)直進する。ProjectileComponent::PierceCountに
    /// PierceCountを設定することで、命中しても消滅せず複数体を貫通する点が
    /// Homing Missile/FireBoltとの差別化点。
    /// </summary>
    struct BoneSpearWeaponData
    {
        int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.4f;     // 発射間隔(秒)。CooldownRateで乗算短縮される
        float       ProjectileSpeed = 45.0f; // 弾速 m/s

        float       BaseDamage = 9.0f;       // Lv1火力（貫通した敵全員に同じ値が入る）
        float       DamagePerLevel = 2.5f;   // レベル毎の火力増加量

        float       BaseExplosionRadius = 4.0f;     // Lv1当たり判定の見た目基準半径(m)。
                                                     // 貫通が売りのためAOE武器より小さめに設定
        float       ExplosionRadiusPerLevel = 0.5f; // レベル毎の見た目基準半径増加量

        float       HitRadiusMultiplier = 1.5f; // 実際の当たり判定半径 = 上記(BaseExplosionRadius系)×この値。
                                                 // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        int         PierceCount = 2;         // 命中してもこの回数だけ消滅せず貫通する(=最大PierceCount+1体まで命中可能)

        float       ProjectileLifeTime = 3.0f; // 何にも当たらなかった場合に消滅するまでの秒数

        float       SearchRadius = 80.0f;    // 発射対象を探す範囲(m)

        float       HeightOffset = 30.0f;    // 発射位置のY座標 = Owner.Position.y + この値(m)

        std::string ProjectileEffectPath;    // 飛翔中エフェクト(.efk)
        std::string ExplosionEffectPath;     // 命中時エフェクト(.efk)

        REFLECT_BEGIN(BoneSpearWeaponData, "bone_spear_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(ProjectileSpeed)
            REFLECT_FIELD_FLOAT(BaseDamage)
            REFLECT_FIELD_FLOAT(DamagePerLevel)
            REFLECT_FIELD_FLOAT(BaseExplosionRadius)
            REFLECT_FIELD_FLOAT(ExplosionRadiusPerLevel)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_INT(PierceCount)
            REFLECT_FIELD_FLOAT(ProjectileLifeTime)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(ProjectileEffectPath)
            REFLECT_FIELD_STR(ExplosionEffectPath)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::BoneSpearWeaponData);
