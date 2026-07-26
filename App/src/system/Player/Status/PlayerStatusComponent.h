#pragma once

#include<Utility/Export/Export.h>
#include<algorithm>

namespace ecs
{
    /// <summary>
    /// 素のステータス(パーク適用前の初期値)。
    /// </summary>
    struct BaseStatus
    {
        float MaxHp = 100.0f;    // 最大体力
        float MoveSpeed = 5.0f;  // 移動速度
        float AtkPower = 10.0f;  // 攻撃力

        // 被ダメージ軽減に使う防御力。PlayerContactDamageSystemの半減点方式(kDefenseHalfPoint)が
        // 基準のため、0のままだとMulDefenseが何倍でも0のままになってしまう
        float Defense = 20.0f;

        float CooldownRate = 1.0f;     // クールダウン倍率(1.0=等倍、<1で短縮)
        float HpRegenPerSecond = 0.0f; // 秒間HP自然回復量(PlayerRegenSystemが加算)
    };

    /// <summary>
    /// パークで積み上がる乗算バフ(1.0=影響なし)。
    /// 例: 防御+20%のパークを取ったら MulDefense += 0.2f
    /// </summary>
    struct StatusModifier
    {
        float MulMaxHp = 1.0f;
        float MulMoveSpeed = 1.0f;
        float MulAtkPower = 1.0f;
        float MulDefense = 1.0f;
        float MulCooldownRate = 1.0f;

        // 1回の発動で攻撃を繰り返す回数の倍率。武器側はecs::combatutil::GetAttackCount()経由で参照する
        float MulAttackCount = 1.0f;
    };

    /// <summary>
    /// 実効ステータス(Base×Modifierの計算結果)。ゲームロジックはこちらを参照する。
    /// Recompute()を呼んだときだけ更新される(毎フレーム計算はしない)。
    /// </summary>
    struct CurrentStatus
    {
        float MaxHp = 100.0f;
        float MoveSpeed = 5.0f;
        float AtkPower = 10.0f;
        float Defense = 0.0f;
        float CooldownRate = 1.0f;
        float HpRegenPerSecond = 0.0f;
        float AttackCountMultiplier = 1.0f;
    };

    /// <summary>
    /// プレイヤーのステータスコンポーネント。
    /// </summary>
    struct ENGINE_API PlayerStatusComponent
    {
        BaseStatus     Base;
        StatusModifier Modifier;
        CurrentStatus  Current;
        float          CurrentHp = 100.0f;

        /// <summary>trueの間は被ダメージを無効化する(必殺技演出中等)</summary>
        bool           IsInvincible = false;

        /// <summary>
        /// 残りの復活回数(パーク「復活」で加算)。HPが0になったとき1以上あれば1消費して
        /// 全回復し、死亡を取り消す(PlayerContactDamageSystemが判定)。
        /// </summary>
        int            ReviveCount = 0;

        /// <summary>Base×ModifierをCurrentへ反映する。パーク適用後に呼ぶこと。</summary>
        void Recompute()
        {
            Current.MaxHp = Base.MaxHp * Modifier.MulMaxHp;
            Current.MoveSpeed = Base.MoveSpeed * Modifier.MulMoveSpeed;
            Current.AtkPower = Base.AtkPower * Modifier.MulAtkPower;
            Current.Defense = Base.Defense * Modifier.MulDefense;

            // 短縮系パークが積み重なっても発射間隔が0以下にならないようクランプする
            constexpr float kMinCooldownRate = 0.1f;
            Current.CooldownRate = std::max(kMinCooldownRate, Base.CooldownRate * Modifier.MulCooldownRate);

            Current.HpRegenPerSecond = Base.HpRegenPerSecond;
            Current.AttackCountMultiplier = Modifier.MulAttackCount;
        }
    };
}
