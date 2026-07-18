#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>

namespace ecs::combatutil
{
    /// <summary>
    /// ownerEntityのPlayerStatusComponentから攻撃力倍率(Current.AtkPower / Base.AtkPower)を求める。
    /// パークによるAtkPower強化(Modifier.MulAtkPower)を各武器のダメージ計算へ反映するために使う
    /// （PlayerMovementSystemが移動速度の倍率をCurrent/Base比で求めているのと同じ方式）。
    /// コンポーネントが無い場合やBase.AtkPowerが0以下の場合は1.0(補正なし)を返す。
    /// </summary>
    float GetAtkPowerMultiplier(entt::registry& registry, entt::entity ownerEntity);

    /// <summary>
    /// ownerEntityのPlayerStatusComponentから、1回の発動で攻撃(Fire/Pulse/Swing/Zap等)を
    /// 何回繰り返すかを求める(パークによるMulAttackCount強化を反映)。整数に丸め、
    /// 極端な積み上げによる負荷・視認性悪化を防ぐため上限でクランプする。
    /// コンポーネントが無い場合は1(補正なし)を返す。
    /// </summary>
    int GetAttackCount(entt::registry& registry, entt::entity ownerEntity);

    /// <summary>
    /// GetAttackCount()で複数回発動する単方向弾の武器が、baseDirectionを中心に扇状へ
    /// 広がる発射方向を計算する(index番目のショット、Y軸周り(XZ平面)の回転)。
    /// count<=1の場合はbaseDirectionをそのまま返す(既存の単発挙動を変えない)。
    /// </summary>
    DirectX::XMFLOAT3 ComputeSpreadDirection(
        const DirectX::XMFLOAT3& baseDirection, int index, int count, float spreadAngleDegrees);

    /// <summary>
    /// ownerEntityのPlayerStatusComponentからクールダウン倍率(Current.CooldownRate)を取得する。
    /// コンポーネントが無い場合は1.0(補正なし)を返す。各武器のクールダウン再設定
    /// (masterData->FireInterval * GetCooldownRate(...))で共通して使う。
    /// </summary>
    float GetCooldownRate(entt::registry& registry, entt::entity ownerEntity);

    /// <summary>
    /// 指定のワールド座標にダメージ数値のポップアップを生成する(DamageNumberSystemが
    /// 上昇・フェードアウトを担当する一時エンティティ)。isPlayerDamage(自分が被弾)かどうかで
    /// 表示色を変える(敵への与ダメージ=黄、プレイヤーの被ダメージ=赤)。
    /// </summary>
    void SpawnDamageNumber(const DirectX::XMFLOAT3& worldPosition, float damage, bool isPlayerDamage);

    /// <summary>
    /// targetEntityがEnemyTagを持ち、EnemyStatusComponentを保持している場合にdamage分だけ
    /// HPを減らし(0未満にはしない)、Transformがあればダメージ数値のポップアップも生成する。
    /// 複数の武器システムで「敵タグ確認→HP減算→ダメージ数値表示」が重複していたため共通化した。
    /// ダメージを適用できた場合はtrueを返す(呼び出し側でノックバック等の追加処理を行うかどうかの
    /// 判定に使える)。
    /// </summary>
    bool ApplyDamageToEnemy(entt::registry& registry, entt::entity targetEntity, float damage);
}
