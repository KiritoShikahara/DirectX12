#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// SingleShot型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    /// </summary>
    struct SingleShotWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.0f;     // 発射間隔(秒)。CooldownRateで乗算短縮される
        float       ProjectileSpeed = 20.0f; // 弾速 m/s

        float       Damage = 5.0f;           // このレベルでの火力

        float       ExplosionRadius = 15.0f; // このレベルでの爆発エフェクトの見た目基準半径(m)。敵3体分程度のサイズを想定

        float       HitRadiusMultiplier = 2.0f; // 実際の当たり判定半径 = ExplosionRadius×この値。
                                                 // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       ProjectileLifeTime = 3.0f; // 何にも当たらなかった場合に消滅するまでの秒数

        float       HeightOffset = 30.0f;    // 発射位置のY座標 = Owner.Position.y + この値(m)。
                                              // デフォルトはプレイヤーの胸あたりの高さ
                                              // (プレイヤーのColliderComponent半径・y=30を目安とする)

        bool        ExplosionAtGroundLevel = false; // trueなら着弾エフェクトのYを地面(0)に固定する。
                                                      // 飛翔中(ProjectileEffectIds)の高さはHeightOffsetのまま変えない

        // エフェクト素材ID(';'区切りで複数指定可、data::EffectAssetData参照)。
        // ecs::effectutil::ResolveEffectIds()でパス文字列へ解決してから使うこと。
        std::string ProjectileEffectIds;    // 飛翔中エフェクト
        std::string ExplosionEffectIds;     // 着弾時の爆発エフェクト

        REFLECT_BEGIN(SingleShotWeaponData, "single_shot_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(ProjectileSpeed)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(ExplosionRadius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ProjectileLifeTime)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_BOOL(ExplosionAtGroundLevel)
            REFLECT_FIELD_STR(ProjectileEffectIds)
            REFLECT_FIELD_STR(ExplosionEffectIds)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::SingleShotWeaponData);
