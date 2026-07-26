#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Cleave型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、FireInterval秒ごとに狙い方向(PlayerAimComponent::Direction)を
    /// 中心としたConeAngleDegrees(半角)の扇状範囲内にいる敵全員へ、即座に近接ダメージと
    /// ノックバックを与える完全自動の武器（CleaveWeaponSystemが担当）。
    /// ノックバックはEnemyKnockbackComponent/EnemyKnockbackSystemが処理する。
    /// </summary>
    struct CleaveWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       Damage = 12.0f;          // このレベルでの火力

        float       Radius = 20.0f;          // このレベルでの扇状射程(m)

        float       HitRadiusMultiplier = 1.2f; // 実際の判定射程 = Radius×この値。
                                                  // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       ConeAngleDegrees = 60.0f; // 狙い方向からの扇状半角(度)。合計はこの2倍が有効角度

        float       KnockbackForce = 30.0f;   // ノックバックの初速(m/s)
        float       KnockbackDuration = 0.25f;// ノックバックが持続する時間(秒)

        float       HeightOffset = 30.0f;    // エフェクトの再生高さ = Owner.Position.y + この値(m)

        // エフェクト素材ID(';'区切りで複数指定可、data::EffectAssetData参照)。
        // ecs::effectutil::ResolveEffectIds()でパス文字列へ解決してから使うこと。
        std::string EffectIds;               // 発動時エフェクト

        REFLECT_BEGIN(CleaveWeaponData, "cleave_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(Radius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(ConeAngleDegrees)
            REFLECT_FIELD_FLOAT(KnockbackForce)
            REFLECT_FIELD_FLOAT(KnockbackDuration)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(EffectIds)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::CleaveWeaponData);
