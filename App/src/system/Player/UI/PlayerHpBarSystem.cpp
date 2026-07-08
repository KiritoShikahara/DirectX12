#include "apppch.h"
#include "PlayerHpBarSystem.h"

#include"PlayerUiTag.h"

#include"../Status/PlayerStatusComponent.h"
#include<Tag/EntityTag.h>

void ecs::PlayerHpBarSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
{
    auto playerView = registry.view<PlayerTag, PlayerStatusComponent>();
    if (playerView.size_hint() == 0)
    {
        return; // プレイヤー不在
    }

    const auto& status = registry.get<PlayerStatusComponent>(*playerView.begin());

    float ratio = (status.Current.MaxHp > 0.0f)
        ? status.CurrentHp / status.Current.MaxHp
        : 0.0f;
    ratio = std::clamp(ratio, 0.0f, 1.0f);

    // Hpバースプライトに反映
    registry.view<PlayerHpBarTag, Sprite>().each(
        [&](Sprite& sprite)
        {
            sprite.FillAmount = ratio;
        });


}
