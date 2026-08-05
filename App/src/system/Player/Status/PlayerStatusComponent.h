#pragma once

#include<Utility/Export/Export.h>
#include<algorithm>

namespace ecs
{
    ///<summary>
    ///素のステータス、パーク適用前の初期値
    ///</summary>
    struct BaseStatus
    {
        float MaxHp = 100.0f;    // 最大体力
        float MoveSpeed = 5.0f;  // 移動速度
        float AtkPower = 10.0f;  // 攻撃力

        ///<summary>
        ///被ダメージ軽減に使う防御力。PlayerContactDamageSystemの半減点方式が基準のため、0のままだとMulDefenseが何倍でも0のままになる
        ///</summary>
        float Defense = 20.0f;

        float CooldownRate = 1.0f;     // クールダウン倍率、1.0で等倍、1未満で短縮
        float HpRegenPerSecond = 0.0f; // 秒間HP自然回復量、PlayerRegenSystemが加算

        ///<summary>
        ///敵からの接触ダメージを受けた直後、この秒数だけ無敵になる。ショップ強化でのみ加算されパークからは未接続
        ///</summary>
        float PostHitInvincibleDuration = 0.0f;
    };

    ///<summary>
    ///パークで積み上がる乗算バフ。1.0が影響なし
    ///</summary>
    struct StatusModifier
    {
        float MulMaxHp = 1.0f;
        float MulMoveSpeed = 1.0f;
        float MulAtkPower = 1.0f;
        float MulDefense = 1.0f;
        float MulCooldownRate = 1.0f;

        // 1回の発動で攻撃を繰り返す回数の倍率。武器側はecs::combatutil::GetAttackCountを経由して参照する
        float MulAttackCount = 1.0f;
    };

    ///<summary>
    ///実効ステータス、BaseとModifierの計算結果。ゲームロジックはこちらを参照し、Recomputeを呼んだときだけ更新される
    ///</summary>
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

    ///<summary>
    ///プレイヤーのステータスコンポーネント
    ///</summary>
    struct ENGINE_API PlayerStatusComponent
    {
        BaseStatus     Base;
        StatusModifier Modifier;
        CurrentStatus  Current;
        float          CurrentHp = 100.0f;

        ///<summary>
        ///trueの間は被ダメージを無効化する。必殺技演出中等
        ///</summary>
        bool           IsInvincible = false;

        ///<summary>
        ///残りの復活回数。パーク復活・ショップ強化復活回数で加算。HPが0になったとき1以上あれば1消費して全回復し死亡を取り消す
        ///</summary>
        int            ReviveCount = 0;

        ///<summary>
        ///被弾後無敵の残り時間、秒。ショップ強化を取っている場合のみ接触ダメージのたびにPlayerContactDamageSystemがリセットする。IsInvincibleとは別枠でOR判定される
        ///</summary>
        float          PostHitInvincibleTimer = 0.0f;

        ///<summary>
        ///必殺技終了直後の無敵残り時間、秒。演出中のIsInvincibleが切れた直後に無防備な隙ができないための猶予。
        ///PlayerUltimateSystemが必殺技終了時に設定し、PlayerContactDamageSystemが毎フレーム減算する
        ///</summary>
        float          PostUltimateInvincibleTimer = 0.0f;

        ///<summary>
        ///BaseとModifierをCurrentへ反映する。パーク適用後に呼ぶこと
        ///</summary>
        void Recompute()
        {
            Current.MaxHp = Base.MaxHp * Modifier.MulMaxHp;
            Current.MoveSpeed = Base.MoveSpeed * Modifier.MulMoveSpeed;
            Current.AtkPower = Base.AtkPower * Modifier.MulAtkPower;
            Current.Defense = Base.Defense * Modifier.MulDefense;

            // 短縮系パークが積み重なっても発射間隔が0以下にならないようクランプする
            constexpr float kMinCooldownRate = 0.35f;
            Current.CooldownRate = std::max(kMinCooldownRate, Base.CooldownRate * Modifier.MulCooldownRate);

            Current.HpRegenPerSecond = Base.HpRegenPerSecond;
            Current.AttackCountMultiplier = Modifier.MulAttackCount;
            Current.PostHitInvincibleDuration = Base.PostHitInvincibleDuration;
        }
    };
}
