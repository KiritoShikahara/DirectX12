#include "apppch.h"
#include "PlayerCombatUtil.h"

#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/UI/DamageNumber/DamageNumberComponent.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Tag/EntityTag.h>

#include<cmath>
#include<algorithm>

namespace
{
    constexpr float kDamageNumberLifetime = 1.0f;
    constexpr float kDamageNumberSize = 28.0f;
    constexpr float kDamageNumberHeightOffset = 30.0f;

    const DirectX::XMFLOAT4 kEnemyDamageColor = { 1.0f, 0.9f, 0.3f, 1.0f };
    const DirectX::XMFLOAT4 kPlayerDamageColor = { 1.0f, 0.3f, 0.3f, 1.0f };

    constexpr int kMaxAttackCount = 6;
    constexpr float kHitSeVolume = 0.2f;
}

namespace ecs::combatutil
{
    float GetAtkPowerMultiplier(entt::registry& registry, entt::entity ownerEntity)
    {
        const auto* status = registry.try_get<ecs::PlayerStatusComponent>(ownerEntity);
        if (status == nullptr || status->Base.AtkPower <= 0.0f) return 1.0f;

        return status->Current.AtkPower / status->Base.AtkPower;
    }

    int GetAttackCount(entt::registry& registry, entt::entity ownerEntity)
    {
        const auto* status = registry.try_get<ecs::PlayerStatusComponent>(ownerEntity);
        if (status == nullptr) return 1;

        const int count = static_cast<int>(std::lround(status->Current.AttackCountMultiplier));
        return std::clamp(count, 1, kMaxAttackCount);
    }

    DirectX::XMFLOAT3 ComputeSpreadDirection(
        const DirectX::XMFLOAT3& baseDirection, int index, int count, float spreadAngleDegrees)
    {
        if (count <= 1) return baseDirection;

        // 中心0度を基準に対称に広がるオフセット角度を求める
        const float offsetDeg = spreadAngleDegrees * (static_cast<float>(index) - static_cast<float>(count - 1) * 0.5f);
        const float rad = DirectX::XMConvertToRadians(offsetDeg);
        const float cosA = std::cos(rad);
        const float sinA = std::sin(rad);

        return
        {
            baseDirection.x * cosA - baseDirection.z * sinA,
            baseDirection.y,
            baseDirection.x * sinA + baseDirection.z * cosA,
        };
    }

    float GetCooldownRate(entt::registry& registry, entt::entity ownerEntity)
    {
        const auto* status = registry.try_get<ecs::PlayerStatusComponent>(ownerEntity);
        return status != nullptr ? status->Current.CooldownRate : 1.0f;
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

    bool ApplyDamageToEnemy(entt::registry& registry, entt::entity targetEntity, float damage)
    {
        if (!registry.all_of<ecs::EnemyTag>(targetEntity)) return false;

        auto* status = registry.try_get<ecs::EnemyStatusComponent>(targetEntity);
        if (status == nullptr) return false;

        // ノックバック等の物理的な反応はさせずHPのみ減少させる。0以下になった後の破棄はEnemyDeathSystemが担当する
        status->CurrentHp = std::max(0.0f, status->CurrentHp - damage);

        if (const auto* transform = registry.try_get<ecs::Transform>(targetEntity))
        {
            SpawnDamageNumber(transform->GetPosition(), damage, false);
        }

        PLAY_SE("Assets/Sound/SE/SE_Hit.aud", false, kHitSeVolume, false);

        return true;
    }
}
