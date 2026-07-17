#pragma once

#include<Utility/Export/Export.h>
#include<algorithm>

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
        // 防御力（被ダメージ軽減に使う。PlayerContactDamageSystemのkDefenseHalfPoint(50)を
        // 基準にした半減点方式）。0のままだとMulDefense(乗算バフ)が何倍しても0のままになり
        // 防御系パークが無意味になるため、非0の初期値を与える
        float Defense = 20.0f;
        float CooldownRate = 1.0f;   // クールダウン倍率（1.0=等倍、<1で短縮）
        float HpRegenPerSecond = 0.0f; // 秒間HP自然回復量(ecs::PlayerRegenSystemが毎フレーム加算)
    };

    /// <summary>
    /// パークで積み上がる乗算バフ（1.0 = 影響なし）。
    /// 例: 防御+20% のパークを取得したら適用側で MulDefense += 0.2f
    /// </summary>
    struct StatusModifier
    {
        float MulMaxHp = 1.0f;
        float MulMoveSpeed = 1.0f;
        float MulAtkPower = 1.0f;
        float MulDefense = 1.0f;
        float MulCooldownRate = 1.0f;

        // 1回の発動で攻撃(Fire/Pulse/Swing/Zap等)を繰り返す回数の倍率。
        // 基準は常に1.0倍(素の状態で1回)のため、他のMul系と違いBaseStatusを持たない。
        // 武器側はこの値を直接使わず ecs::combatutil::GetAttackCount() 経由で参照する
        // (整数丸め・上限クランプをそこに集約するため)。
        float MulAttackCount = 1.0f;
    };

    /// <summary>
    /// 実効ステータス（Base × Modifier の再計算結果）。
    /// ゲームロジックはこの値を参照する。毎フレーム計算せず、
    /// パーク取得時などに Recompute() でのみ更新する。
    /// </summary>
    struct CurrentStatus
    {
        float MaxHp = 100.0f;
        float MoveSpeed = 5.0f;
        float AtkPower = 10.0f;
        float Defense = 0.0f;
        float CooldownRate = 1.0f;
        float HpRegenPerSecond = 0.0f;

        // Modifier.MulAttackCountをそのまま反映(基準は常に1.0倍のため乗算元のBaseを持たない)
        float AttackCountMultiplier = 1.0f;
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

        /// <summary>true の間、被ダメージ判定を無効化する（必殺技演出中等）</summary>
        bool           IsInvincible = false;

        /// <summary>
        /// パーク適用後に呼ぶ：Base × Modifier → Current。
        /// 生成直後にも一度呼び、CurrentHp = Current.MaxHp で初期化すること。
        /// </summary>
        void Recompute()
        {
            Current.MaxHp = Base.MaxHp * Modifier.MulMaxHp;
            Current.MoveSpeed = Base.MoveSpeed * Modifier.MulMoveSpeed;
            Current.AtkPower = Base.AtkPower * Modifier.MulAtkPower;
            Current.Defense = Base.Defense * Modifier.MulDefense;

            // クールダウン短縮系パークが積み重なっても0以下(発射間隔が0や負)にならないようクランプする
            constexpr float kMinCooldownRate = 0.1f;
            Current.CooldownRate = std::max(kMinCooldownRate, Base.CooldownRate * Modifier.MulCooldownRate);

            // 現状パークによる倍率は無いため素の値をそのまま反映する
            // (将来、回復速度アップ等のパークを追加する場合はMulHpRegenを新設してここに掛ける)
            Current.HpRegenPerSecond = Base.HpRegenPerSecond;

            Current.AttackCountMultiplier = Modifier.MulAttackCount;
        }
    };
}