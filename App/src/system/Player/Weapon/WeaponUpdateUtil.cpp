#include "apppch.h"
#include "WeaponUpdateUtil.h"

#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/PlayerActionLock.h>
#include<Scene/Game/State/GameState.h>

namespace ecs::weaponutil
{
    bool IsInGame(entt::registry& registry)
    {
        auto stateView = registry.view<::ecs::GameStateComponent>();
        if (stateView.begin() == stateView.end()) return false;

        return registry.get<::ecs::GameStateComponent>(*stateView.begin()).GameState == ::sys::eGameState::InGame;
    }

    bool ShouldSkipAutoWeaponUpdate(entt::registry& registry)
    {
        return !IsInGame(registry) || ecs::IsPlayerUltimateActive(registry) || ecs::IsOptionsMenuOpen(registry);
    }

    bool ShouldSkipManualWeaponUpdate(entt::registry& registry)
    {
        return !IsInGame(registry) || ecs::IsPlayerActionLocked(registry) || ecs::IsOptionsMenuOpen(registry);
    }

    int ComputeWeaponDataId(const ecs::WeaponComponent& weapon)
    {
        return (weapon.WeaponID + 1) * 1000 + weapon.Level;
    }
}
