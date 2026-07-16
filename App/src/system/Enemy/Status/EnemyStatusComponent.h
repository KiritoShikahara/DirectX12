#pragma once

#include<Utility/Export/Export.h>

namespace ecs
{
    /// <summary>
    /// 敵の基礎ステータス（マスタデータ由来の不変値）。
    /// ウェーブ強化前の素の値を保持する。
    /// </summary>
    struct ENGINE_API EnemyBaseStatus
    {
        float MaxHp = 10.0f;     // 最大HP
        float MoveSpeed = 2.0f;  // 移動速度 m/s
        float AtkPower = 1.0f;   // 接触ダメージ
        float ExperienceValue = 1.0f; // 撃破時にプレイヤーへ与える経験値(EnemyData.csvのExpと対応)
        float GoldValue = 3.0f;       // 撃破時にプレイヤーへ与えるゴールド(ExperienceValueと同様、
                                       // 現状EnemyData.csvには未接続。ボースはGameSceneFactoryで倍率適用)
    };

    /// <summary>
    /// ウェーブ進行による強化率（乗算のみ・1.0基準）。
    /// プレイヤーのパーク強化と同様、ウェーブごとに加算合成する。
    /// 例: 1ウェーブ +10% なら MulMaxHp += 0.1f を進行のたびに加算。
    /// </summary>
    struct ENGINE_API EnemyWaveModifier
    {
        float MulMaxHp = 1.0f;
        float MulMoveSpeed = 1.0f;
        float MulAtkPower = 1.0f;
    };

    /// <summary>
    /// Base × WaveModifier の確定結果キャッシュ。
    /// 毎フレーム計算せず、ウェーブ変化時の Recompute() でのみ更新する。
    /// </summary>
    struct ENGINE_API EnemyCurrentStatus
    {
        float MaxHp = 10.0f;
        float MoveSpeed = 2.0f;
        float AtkPower = 1.0f;
    };

    /// <summary>
    /// 敵のステータスコンポーネント。
    /// Base（不変） / WaveModifier（ウェーブ強化率） / Current（確定値） / CurrentHp（現在HP）。
    /// </summary>
    struct ENGINE_API EnemyStatusComponent
    {
		int EnemyId = 0; // 敵の種類ID。マスタデータのIDと対応する。

        EnemyBaseStatus    Base;
        EnemyWaveModifier  WaveMod;
        EnemyCurrentStatus Current;

        /// <summary>現在HP。生成直後は Recompute() → CurrentHp = Current.MaxHp で初期化する。</summary>
        float CurrentHp = 10.0f;

        /// <summary>Base × WaveModifier を計算して Current に反映する。</summary>
        void Recompute()
        {
            Current.MaxHp = Base.MaxHp * WaveMod.MulMaxHp;
            Current.MoveSpeed = Base.MoveSpeed * WaveMod.MulMoveSpeed;
            Current.AtkPower = Base.AtkPower * WaveMod.MulAtkPower;
        }
    };
}