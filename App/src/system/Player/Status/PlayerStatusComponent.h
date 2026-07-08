#pragma once

#include<Utility/Export/Export.h>

namespace ecs
{
    /// <summary>
    /// 素のステータス（パーク適用の基準となる初期値）。
    /// </summary>
    struct BaseStatus
    {
        float MaxHp = 100.0f; // 最大体力
        float MoveSpeed = 5.0f;   // 移動速度
        float AtkPower = 10.0f;  // 攻撃力
        float CooldownRate = 1.0f;   // クールダウン倍率（1.0=等倍、<1で短縮）
    };

    /// <summary>
    /// パークで積み上がる乗算バフ（1.0 = 影響なし）。
    /// </summary>
    struct StatusModifier
    {
        float MulMaxHp = 1.0f;
        float MulMoveSpeed = 1.0f;
        float MulAtkPower = 1.0f;
        float MulCooldownRate = 1.0f;
    };

    /// <summary>
    /// 実効ステータス（Base × Modifier の再計算結果）。
    /// </summary>
    struct CurrentStatus
    {
        float MaxHp = 100.0f;
        float MoveSpeed = 5.0f;
        float AtkPower = 10.0f;
        float CooldownRate = 1.0f;
    };

    /// <summary>
    /// プレイヤーのステータスコンポーネント。
    /// </summary>
    struct ENGINE_API PlayerStatusComponent
    {
        BaseStatus     Base;      // 素の値
        StatusModifier Modifier;  // パークで積み上がる乗算バフ
        CurrentStatus  Current;   // Recompute() の結果（参照用）
        float          CurrentHp = 100.0f; // 現在体力

        /// <summary>
        /// パーク適用後に呼ぶ：Base × Modifier → Current。
        /// 生成直後にも一度呼び、CurrentHp = Current.MaxHp で初期化すること。
        /// MaxHp を上げるパーク適用時は、旧 Current.MaxHp を控えて
        /// 差分を CurrentHp に加算するなどのルールは適用側で決める。
        /// </summary>
        void Recompute()
        {
            Current.MaxHp = Base.MaxHp * Modifier.MulMaxHp;
            Current.MoveSpeed = Base.MoveSpeed * Modifier.MulMoveSpeed;
            Current.AtkPower = Base.AtkPower * Modifier.MulAtkPower;
            Current.CooldownRate = Base.CooldownRate * Modifier.MulCooldownRate;
        }
    };
}