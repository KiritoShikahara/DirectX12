#pragma once

#include<Utility/Export/Export.h>

namespace ecs
{
    ///<summary>
    ///敵の基礎ステータス、マスタデータ由来の不変値。ウェーブ強化前の素の値を保持する
    ///</summary>
    struct ENGINE_API EnemyBaseStatus
    {
        float MaxHp = 10.0f;     // 最大HP
        float MoveSpeed = 2.0f;  // 移動速度、m/s
        float AtkPower = 1.0f;   // 接触ダメージ
        float ExperienceValue = 1.0f; // 撃破時にプレイヤーへ与える経験値
        float GoldValue = 3.0f;       // 撃破時にプレイヤーへ与えるゴールド
    };

    ///<summary>
    ///ウェーブ進行による強化率、乗算のみで1.0基準。プレイヤーのパーク強化と同様、ウェーブごとに加算合成する
    ///</summary>
    struct ENGINE_API EnemyWaveModifier
    {
        float MulMaxHp = 1.0f;
        float MulMoveSpeed = 1.0f;
        float MulAtkPower = 1.0f;
    };

    ///<summary>
    ///BaseとWaveModifierの確定結果キャッシュ。毎フレーム計算せずウェーブ変化時のRecomputeでのみ更新する
    ///</summary>
    struct ENGINE_API EnemyCurrentStatus
    {
        float MaxHp = 10.0f;
        float MoveSpeed = 2.0f;
        float AtkPower = 1.0f;
    };

    ///<summary>
    ///敵のステータスコンポーネント。Base、WaveMod、Current、CurrentHpで構成する
    ///</summary>
    struct ENGINE_API EnemyStatusComponent
    {
		int EnemyId = 0; // 敵の種類ID。マスタデータのIdと対応する

        EnemyBaseStatus    Base;
        EnemyWaveModifier  WaveMod;
        EnemyCurrentStatus Current;

        ///<summary>
        ///現在HP。生成直後のRecompute→CurrentHp = Current.MaxHpで初期化する
        ///</summary>
        float CurrentHp = 10.0f;

        ///<summary>
        ///必殺技の範囲ダメージを受けたか。EnemyDeathSystemがこれを見て必殺技ゲージを二重加算しないようにする。ゴールドと経験値は通常撃破と同様に加算される
        ///</summary>
        bool DamagedByUltimate = false;

        ///<summary>
        ///BaseとWaveModifierを計算してCurrentに反映する
        ///</summary>
        void Recompute()
        {
            Current.MaxHp = Base.MaxHp * WaveMod.MulMaxHp;
            Current.MoveSpeed = Base.MoveSpeed * WaveMod.MulMoveSpeed;
            Current.AtkPower = Base.AtkPower * WaveMod.MulAtkPower;
        }
    };
}
