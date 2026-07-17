#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Void Beam型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の最も近い敵の方向へ
    /// 直線状のビームを放つ完全自動の武器（VoidBeamWeaponSystemが担当）。
    /// Chain Lightningと同様、移動する実体を持たないため命中は瞬時に解決される
    /// （Projectile汎用パイプラインは使わない）。BeamLength・BeamWidthで定義される
    /// 直線状の範囲内にいる敵は、跳躍(Chain)のような対象数制限・減衰を挟まず全員が
    /// 貫通ヒットする点がChain Lightningとの差別化点。
    /// </summary>
    struct VoidBeamWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.8f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       Damage = 7.0f;           // このレベルでの火力（貫通ヒットする全員に同じ値が入る）

        float       SearchRadius = 90.0f;    // 狙い方向(最も近い敵)を探す範囲(m)
        float       BeamLength = 90.0f;      // ビームの直線距離(m)
        float       BeamWidth = 6.0f;        // ビームの判定太さ(中心線からの許容垂線距離、m)

        float       HeightOffset = 30.0f;    // ヒットエフェクトの再生高さ = 対象のY座標 + この値(m)

        std::string HitEffectPath;           // 命中のたびに1回だけ再生するエフェクト(.efk)
        float       HitEffectScale = 1.0f;   // ヒットエフェクトの見た目倍率

        REFLECT_BEGIN(VoidBeamWeaponData, "void_beam_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(SearchRadius)
            REFLECT_FIELD_FLOAT(BeamLength)
            REFLECT_FIELD_FLOAT(BeamWidth)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(HitEffectPath)
            REFLECT_FIELD_FLOAT(HitEffectScale)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::VoidBeamWeaponData);
