#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// Flicker Strike型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
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
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       FireInterval = 1.2f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

        float       Damage = 15.0f;          // 1ヒットあたりのこのレベルでの火力(初撃・追加ワープとも共通)。
                                              // 序盤の雑魚敵(MaxHp10前後)を確実に一撃で倒せる値が目安

        int         ChargeHitCount = 2;      // パワーチャージ1個につき発生する追加ワープ攻撃の回数
        int         MaxCharge = 5;           // パワーチャージの上限の初期値(プレイヤーLv1時点)。
                                              // 実際の上限 = MaxCharge + MaxChargePerLevel×(Lv-1)
                                              // (ecs::ComputeMaxPowerCharge参照)
        int         MaxChargePerLevel = 1;   // プレイヤーが1レベル上がるごとに上限へ加算される数
        int         InitialCharge = 3;       // ゲーム開始時に所持しているパワーチャージ数

        float       InitialTargetMaxRange = 60.0f; // 最初の対象を探す、狙い方向への最大距離(m)
        float       InitialSearchWidth = 6.0f;     // 最初の対象を探す、狙い方向の直線の半幅(m)
        float       WarpSearchRadius = 200.0f;     // 追加ワープ攻撃の対象を探す範囲(m、直前の着地点基準)。
                                                    // 初撃の直線探索より広く取り、周辺の敵を拾いやすくする
        float       WarpInterval = 0.08f;          // ワープ攻撃1回ごとの間隔(秒)
        float       TeleportOffset = 3.0f;         // 対象からどれだけ離れた位置へワープするか(m)

        float       HeightOffset = 30.0f;    // ヒットエフェクトの再生高さ = 対象のY座標 + この値(m)

        std::string HitEffectPath;           // 命中のたびに1回だけ再生する被弾エフェクト(.efk、
                                              // ';'区切りで複数指定可)。雷を思わせる見た目にする
                                              // ため、既定でLightningStrike.efk(雷本体)+
                                              // Light4.efk(閃光)を組み合わせている
        float       HitEffectScale = 7.0f;   // ヒットエフェクトの見た目倍率(派手さの要望により拡大)

        REFLECT_BEGIN(FlickerStrikeWeaponData, "flicker_strike_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(FireInterval)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_INT(ChargeHitCount)
            REFLECT_FIELD_INT(MaxCharge)
            REFLECT_FIELD_INT(MaxChargePerLevel)
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

    /// <summary>
    /// MaxCharge/MaxChargePerLevel/InitialChargeはレベル非依存(全レベル行で同じ値)の
    /// 武器グローバル設定のため、これらを参照する側(ecs::ComputeMaxPowerCharge、
    /// GameSceneFactory::CreatePlayer)は武器インスタンスのLevelに関わらず常にこのId
    /// (WeaponID=0のLv1行)を使う。Id=0(旧方式)へのGetByIdは行スキーマ変更後に必ず
    /// nullptrになるため使用しないこと。
    /// </summary>
    constexpr int kFlickerStrikeGlobalConfigId = 1001;
}

REFLECT_REGISTER(data::FlickerStrikeWeaponData);
