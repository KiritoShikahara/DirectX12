#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Ricochet型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の最も近い敵へ自動で
    /// 球体の弾を発射する完全自動の武器（RicochetWeaponSystemが担当）。
    /// 敵に命中するとダメージ+ヒットエフェクトを与え、その場でSplitCount体の子弾に
    /// 増殖する(命中した敵を除くSplitSearchRadius内の別の敵へ、それぞれ飛んでいく)。
    /// 子弾もさらに命中すれば同様に増殖するが、世代(Generation)がMaxGenerationに
    /// 達した弾は増殖せず、通常の弾と同じく命中で消滅する(増殖の無限連鎖を防ぐ)。
    /// 弾自体は飛翔中はプリミティブの球体のみを表示し、エフェクトは命中時にのみ再生する。
    /// 移動・命中判定・増殖は既存のProjectile汎用パイプライン(ProjectileMovementSystem/
    /// ProjectileCollisionSystem)をそのまま再利用する。
    /// </summary>
    struct RicochetWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.0f;     // 発射間隔(秒)。CooldownRateで乗算短縮される

        float       ProjectileSpeed = 40.0f; // 弾速 m/s
        float       Damage = 6.0f;           // このレベルでの火力(初弾・子弾とも同じ値を使う)

        float       Radius = 1.0f;           // 球体の見た目半径(m)。当たり判定の基準にも使う
        float       HitRadiusMultiplier = 1.5f; // 実際の当たり判定半径 = Radius×この値

        float       ProjectileLifeTime = 5.0f; // 何にも当たらなかった場合に消滅するまでの秒数

        float       SearchRadius = 60.0f;    // 初弾の対象探索範囲(プレイヤー基準、m)

        int         SplitCount = 2;          // 命中時に増殖する子弾の数(このレベルでの値)
        int         MaxGeneration = 5;       // 増殖(反射)できる世代の上限、既定5(このレベルでの値、外部データ管理)。
                                              // 初弾はGeneration=0で、命中して増殖した子弾はGeneration+1になる
        float       SplitSearchRadius = 30.0f; // 子弾の対象(命中した敵を除く)を探す範囲(m)

        float       HeightOffset = 30.0f;    // 発射位置のY座標 = Owner.Position.y + この値(m)

        // エフェクト素材ID(';'区切りで複数指定可、data::EffectAssetData参照)。
        // ecs::effectutil::ResolveEffectIds()でパス文字列へ解決してから使うこと。
        // 飛翔中は球体のみを表示し、このエフェクトは命中の瞬間にだけ再生する。
        std::string HitEffectIds;

        REFLECT_BEGIN(RicochetWeaponData, "ricochet_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(ProjectileSpeed)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(Radius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ProjectileLifeTime)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_INT(SplitCount)
            REFLECT_FIELD_INT(MaxGeneration)
            REFLECT_FIELD_FLOAT(SplitSearchRadius)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(HitEffectIds)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::RicochetWeaponData);
