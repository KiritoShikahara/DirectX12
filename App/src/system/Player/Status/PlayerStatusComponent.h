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

        // 敵からの接触ダメージを受けた直後、この秒数だけ無敵になる(PlayerContactDamageSystemが管理)。
        // ショップ強化(data::eStatUpgradeType::PostHitInvincibility)でのみ加算される(パークからは未接続)
        float PostHitInvincibleDuration = 0.0f;
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
        float PostHitInvincibleDuration = 0.0f;
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
        /// 残りの復活回数(パーク「復活」・ショップ強化「復活回数」で加算)。HPが0になったとき
        /// 1以上あれば1消費して全回復し、死亡を取り消す(PlayerContactDamageSystemが判定)。
        /// </summary>
        int            ReviveCount = 0;

        /// <summary>
        /// 被弾後無敵の残り時間(秒)。Current.PostHitInvincibleDuration > 0のショップ強化を
        /// 取っている場合のみ、接触ダメージを受けるたびにPlayerContactDamageSystemが
        /// この値をCurrent.PostHitInvincibleDurationへリセットする。IsInvincibleとは別枠
        /// (Ultimate/Flicker Strikeの排他演出用フラグと衝突させないため、OR判定で扱う)。
        /// </summary>
        float          PostHitInvincibleTimer = 0.0f;

        /// <summary>Base×ModifierをCurrentへ反映する。パーク適用後に呼ぶこと。</summary>
        void Recompute()
        {
            Current.MaxHp = Base.MaxHp * Modifier.MulMaxHp;
            Current.MoveSpeed = Base.MoveSpeed * Modifier.MulMoveSpeed;
            Current.AtkPower = Base.AtkPower * Modifier.MulAtkPower;
            Current.Defense = Base.Defense * Modifier.MulDefense;

            // 短縮系パークが積み重なっても発射間隔が0以下にならないようクランプする。
            // 0.1(10倍速)だと短縮パーク(CooldownDown/Berserk/AllStatsUp)が重なった際に
            // 攻撃間隔が実質無くなってしまうため、0.35(約2.9倍速が上限)まで引き上げた
            constexpr float kMinCooldownRate = 0.35f;
            Current.CooldownRate = std::max(kMinCooldownRate, Base.CooldownRate * Modifier.MulCooldownRate);

            Current.HpRegenPerSecond = Base.HpRegenPerSecond;
            Current.AttackCountMultiplier = Modifier.MulAttackCount;
            Current.PostHitInvincibleDuration = Base.PostHitInvincibleDuration;
        }
    };
}
