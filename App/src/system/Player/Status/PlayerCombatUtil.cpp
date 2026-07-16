#include "apppch.h"
#include "PlayerCombatUtil.h"

#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/UI/DamageNumber/DamageNumberComponent.h>

#include<cmath>

namespace
{
    // ポップアップの表示時間(秒)・見た目のサイズ・高さオフセットの暫定値
    constexpr float kDamageNumberLifetime = 1.0f;
    constexpr float kDamageNumberSize = 28.0f;
    constexpr float kDamageNumberHeightOffset = 30.0f; // 対象の胸あたりの高さ目安(他の武器と同じ基準)

    const DirectX::XMFLOAT4 kEnemyDamageColor = { 1.0f, 0.9f, 0.3f, 1.0f }; // 敵への与ダメージ: 黄
    const DirectX::XMFLOAT4 kPlayerDamageColor = { 1.0f, 0.3f, 0.3f, 1.0f }; // プレイヤーの被ダメージ: 赤
}

namespace ecs::combatutil
{
    float GetAtkPowerMultiplier(entt::registry& registry, entt::entity ownerEntity)
    {
        const auto* status = registry.try_get<ecs::PlayerStatusComponent>(ownerEntity);
        if (status == nullptr || status->Base.AtkPower <= 0.0f) return 1.0f;

        return status->Current.AtkPower / status->Base.AtkPower;
    }

    void SpawnDamageNumber(const DirectX::XMFLOAT3& worldPosition, float damage, bool isPlayerDamage)
    {
        auto& manager = ::ecs::EntityManager::Get();
        auto entity = manager.CreateEntity();

        auto& number = manager.AddComponent<ecs::DamageNumberComponent>(entity);
        number.WorldPosition = { worldPosition.x, worldPosition.y + kDamageNumberHeightOffset, worldPosition.z };
        number.RemainingTime = kDamageNumberLifetime;
        number.TotalTime = kDamageNumberLifetime;

        auto& text = manager.AddComponent<ecs::TextComponent>(entity);
        text.Text = std::to_wstring(static_cast<long long>(std::lround(damage)));
        text.Size = kDamageNumberSize;
        text.Layer = 20;
        text.Color = isPlayerDamage ? kPlayerDamageColor : kEnemyDamageColor;
    }
}
