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
    /// 指定のワールド座標にダメージ数値のポップアップを生成する(DamageNumberSystemが
    /// 上昇・フェードアウトを担当する一時エンティティ)。isPlayerDamage(自分が被弾)かどうかで
    /// 表示色を変える(敵への与ダメージ=黄、プレイヤーの被ダメージ=赤)。
    /// </summary>
    void SpawnDamageNumber(const DirectX::XMFLOAT3& worldPosition, float damage, bool isPlayerDamage);
}
