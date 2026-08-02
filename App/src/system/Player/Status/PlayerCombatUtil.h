#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>

namespace ecs::combatutil
{
    ///<summary>
    ///ownerEntityのPlayerStatusComponentから攻撃力倍率を求める。Current.AtkPower/Base.AtkPower。コンポーネントが無い場合やBase.AtkPowerが0以下の場合は1.0を返す
    ///</summary>
    float GetAtkPowerMultiplier(entt::registry& registry, entt::entity ownerEntity);

    ///<summary>
    ///ownerEntityのPlayerStatusComponentから、1回の発動で攻撃を何回繰り返すかを求める。整数に丸めて上限でクランプし、コンポーネントが無い場合は1を返す
    ///</summary>
    int GetAttackCount(entt::registry& registry, entt::entity ownerEntity);

    ///<summary>
    ///複数回発動する単方向弾の武器が、baseDirectionを中心に扇状へ広がる発射方向を計算する。count<=1の場合はbaseDirectionをそのまま返す
    ///</summary>
    DirectX::XMFLOAT3 ComputeSpreadDirection(
        const DirectX::XMFLOAT3& baseDirection, int index, int count, float spreadAngleDegrees);

    ///<summary>
    ///ownerEntityのPlayerStatusComponentからクールダウン倍率を取得する。コンポーネントが無い場合は1.0を返す
    ///</summary>
    float GetCooldownRate(entt::registry& registry, entt::entity ownerEntity);

    ///<summary>
    ///指定のワールド座標にダメージ数値のポップアップを生成する。isPlayerDamageに応じて表示色を変える
    ///</summary>
    void SpawnDamageNumber(const DirectX::XMFLOAT3& worldPosition, float damage, bool isPlayerDamage);

    ///<summary>
    ///targetEntityが敵ならHPをdamage分減らしポップアップも生成する。適用できた場合はtrueを返す
    ///</summary>
    bool ApplyDamageToEnemy(entt::registry& registry, entt::entity targetEntity, float damage);
}
