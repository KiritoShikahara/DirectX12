#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Chain Lightning型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
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
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       Damage = 6.0f;           // 最初の命中でのこのレベルでの火力
        float       DamageFalloffPerJump = 0.75f; // 1回跳ねるごとにダメージへ乗算する減衰率

        float       SearchRadius = 80.0f;    // 最初の対象をプレイヤーから探す範囲(m)
        float       JumpRadius = 25.0f;      // 直前の対象から次の対象を探す範囲(m)
        int         MaxJumps = 3;            // 最初の1体を除き、追加で跳ねる最大回数

        float       HeightOffset = 30.0f;    // ヒットエフェクトの再生高さ = 対象のY座標 + この値(m)

        // エフェクト素材ID(';'区切りで複数指定可、data::EffectAssetData参照)。
        // ecs::effectutil::ResolveEffectIds()でパス文字列へ解決してから使うこと。
        std::string HitEffectIds;           // 命中のたびに1回だけ再生するエフェクト
        float       HitEffectScale = 4.0f;   // ヒットエフェクトの見た目倍率（代用素材で視認しづらいため拡大）

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
