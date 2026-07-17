#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Flicker Strike型武器のマスタデータ（CSV/DB）。
    /// WeaponComponent::WeaponID と対応する。
    /// ダメージは Base + PerLevel * (Level - 1) の線形成長とする（他のWeaponDataと同じ方式）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 狙い方向(PlayerAimComponent::Direction、他の武器と同じ基準)へ、InitialTargetMaxRange・
    /// InitialSearchWidthで定義される直線範囲内の最も近い敵を探し、ワープして初撃を与える。
    /// 方向上に敵がいなければ何も起きない(クールダウン消費なし)。命中した場合のみ、
    /// 所持しているパワーチャージ(PlayerPowerChargeComponent)を全消費して追加のワープ攻撃を
    /// 行う(FlickerStrikeWeaponSystemが担当)。追加攻撃はチャージ1個につきChargeHitCount回、
    /// WarpInterval秒間隔でWarpSearchRadius内の近くの敵(直前の対象は除く)へ次々ワープする。
    /// 近くに対象がいなくなった時点で残り回数は打ち切られる(消費したチャージは戻らない)。
    /// ワープ演出中に他の手動スキル(Attack/Attack2/Ultimate)を入力すると、その時点でシーケンスを
    /// 打ち切る(プレイヤーの操作意思を優先する)。MaxChargeはこの武器がパワーチャージの
    /// 現状唯一の消費先のためここで管理する。
    /// </summary>
    struct FlickerStrikeWeaponData
    {
        int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       BaseDamage = 15.0f;      // 1ヒットあたりのLv1火力(初撃・追加ワープとも共通)。
                                              // 序盤の雑魚敵(MaxHp10前後)を確実に一撃で倒せる値
        float       DamagePerLevel = 2.5f;   // レベル毎の火力増加量

        int         ChargeHitCount = 2;      // パワーチャージ1個につき発生する追加ワープ攻撃の回数
        int         MaxCharge = 5;           // パワーチャージの上限(PlayerPowerChargeComponent側)
        int         InitialCharge = 3;       // ゲーム開始時に所持しているパワーチャージ数

        float       InitialTargetMaxRange = 60.0f; // 最初の対象を探す、狙い方向への最大距離(m)
        float       InitialSearchWidth = 6.0f;     // 最初の対象を探す、狙い方向の直線の半幅(m)
        float       WarpSearchRadius = 200.0f;     // 追加ワープ攻撃の対象を探す範囲(m、直前の着地点基準)。
                                                    // 初撃の直線探索より広く取り、周辺の敵を拾いやすくする
        float       WarpInterval = 0.08f;          // ワープ攻撃1回ごとの間隔(秒)
        float       TeleportOffset = 3.0f;         // 対象からどれだけ離れた位置へワープするか(m)

        float       HeightOffset = 30.0f;    // ヒットエフェクトの再生高さ = 対象のY座標 + この値(m)

        std::string HitEffectPath;           // 命中のたびに1回だけ再生する被弾エフェクト(.efk)
        float       HitEffectScale = 1.0f;   // ヒットエフェクトの見た目倍率

        REFLECT_BEGIN(FlickerStrikeWeaponData, "flicker_strike_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(BaseDamage)
            REFLECT_FIELD_FLOAT(DamagePerLevel)
            REFLECT_FIELD_INT(ChargeHitCount)
            REFLECT_FIELD_INT(MaxCharge)
            REFLECT_FIELD_INT(InitialCharge)
            REFLECT_FIELD_FLOAT(InitialTargetMaxRange)
            REFLECT_FIELD_FLOAT(InitialSearchWidth)
            REFLECT_FIELD_FLOAT(WarpSearchRadius)
            REFLECT_FIELD_FLOAT(WarpInterval)
            REFLECT_FIELD_FLOAT(TeleportOffset)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(HitEffectPath)
            REFLECT_FIELD_FLOAT(HitEffectScale)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::FlickerStrikeWeaponData);
