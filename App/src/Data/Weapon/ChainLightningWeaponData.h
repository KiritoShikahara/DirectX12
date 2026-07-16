#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Chain Lightning型武器のマスタデータ（CSV/DB）。
    /// WeaponComponent::WeaponID と対応する。
    /// ダメージは Base + PerLevel * (Level - 1) の線形成長とする（他のWeaponDataと同じ方式）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の最も近い敵へ雷撃を放つ
    /// 完全自動の武器（ChainLightningWeaponSystemが担当）。命中した敵から
    /// JumpRadius内にいる未命中の最も近い敵へ、最大MaxJumps回まで自動的に跳ね移る。
    /// 跳ねるたびにダメージへDamageFalloffPerJumpを乗算する（1回跳ねるごとに減衰する）。
    /// 移動する実体を持たないため、命中は瞬時に解決される（Projectile汎用パイプラインは使わない）。
    /// </summary>
    struct ChainLightningWeaponData
    {
        int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       BaseDamage = 6.0f;       // 最初の命中のLv1火力
        float       DamagePerLevel = 1.5f;   // レベル毎の火力増加量
        float       DamageFalloffPerJump = 0.75f; // 1回跳ねるごとにダメージへ乗算する減衰率

        float       SearchRadius = 80.0f;    // 最初の対象をプレイヤーから探す範囲(m)
        float       JumpRadius = 25.0f;      // 直前の対象から次の対象を探す範囲(m)
        int         MaxJumps = 3;            // 最初の1体を除き、追加で跳ねる最大回数

        float       HeightOffset = 30.0f;    // ヒットエフェクトの再生高さ = 対象のY座標 + この値(m)

        std::string HitEffectPath;           // 命中のたびに1回だけ再生するエフェクト(.efk)
        float       HitEffectScale = 4.0f;   // ヒットエフェクトの見た目倍率（代用素材で視認しづらいため拡大）

        REFLECT_BEGIN(ChainLightningWeaponData, "chain_lightning_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(BaseDamage)
            REFLECT_FIELD_FLOAT(DamagePerLevel)
            REFLECT_FIELD_FLOAT(DamageFalloffPerJump)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_FLOAT(JumpRadius)
            REFLECT_FIELD_INT(MaxJumps)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(HitEffectPath)
            REFLECT_FIELD_FLOAT(HitEffectScale)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::ChainLightningWeaponData);
