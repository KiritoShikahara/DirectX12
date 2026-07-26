#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Nova型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 発動トリガーが無く、PulseInterval秒ごとにプレイヤー自身を中心とした円形範囲へ
    /// 即座にダメージを与える（NovaWeaponSystemが担当）。狙い・移動を必要としない
    /// 完全自動の持続武器（パーク選択でのみ取得可能）。
    /// </summary>
    struct NovaWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       PulseInterval = 1.5f;    // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       Damage = 10.0f;          // このレベルでの火力

        float       Radius = 15.0f;          // このレベルでのエフェクト見た目基準半径(m)

        float       HitRadiusMultiplier = 2.0f; // 実際の当たり判定半径 = Radius×この値。
                                                 // エフェクトの見た目サイズは変えず、判定だけ拡大するため分離

        float       HeightOffset = 30.0f;    // 発生位置のY座標 = Owner.Position.y + この値(m)

        // エフェクト素材ID(';'区切りで複数指定可、data::EffectAssetData参照)。
        // ecs::effectutil::ResolveEffectIds()でパス文字列へ解決してから使うこと。
        std::string EffectIds;               // 発動時エフェクト

        REFLECT_BEGIN(NovaWeaponData, "nova_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(PulseInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(Radius)
            REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(EffectIds)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::NovaWeaponData);
