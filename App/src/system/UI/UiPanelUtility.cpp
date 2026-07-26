#include "apppch.h"
#include "UiPanelUtility.h"

namespace ecs::uiutil
{
	entt::entity CreateTranslucentPanel(
		float centerX, float centerY,
		float width, float height,
		int layerOffset,
		float alpha)
	{
		auto& manager = ENTITY_MANAGER;
		auto entity = manager.CreateEntity();

		auto& transform = manager.AddComponent<::ecs::Transform>(entity);
		transform.Set2DPosition(centerX, centerY);

		auto texture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Effect/Texture/White.png");
		auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, texture);
		sprite.Pivot = { 0.5f, 0.5f };
		sprite.Size = { width, height };
		sprite.Color = ::graphics::Color(0.0f, 0.0f, 0.0f, alpha);
		sprite.SetLayer(::ecs::SpriteLayer::UI, layerOffset);

		return entity;
	}
}
