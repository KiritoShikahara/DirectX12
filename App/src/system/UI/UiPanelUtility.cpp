#include "apppch.h"
#include "UiPanelUtility.h"

#include<graphics/Text/Renderer/TextRenderer.h>

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

	std::vector<entt::entity> CreateTextLines(
		const std::vector<std::wstring>& lines,
		eTextHorizontalAlign align,
		float x, float startY, float lineSpacing,
		float size, const DirectX::XMFLOAT4& color, int layer)
	{
		auto& manager = ENTITY_MANAGER;
		auto& textRenderer = ::graphics::TextRenderer::Get();

		std::vector<entt::entity> entities;
		entities.reserve(lines.size());

		for (size_t i = 0; i < lines.size(); ++i)
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = lines[i];
			text.X = (align == eTextHorizontalAlign::Center)
				? (x - textRenderer.MeasureWidth(lines[i], size) * 0.5f)
				: x;
			text.Y = startY + static_cast<float>(i) * lineSpacing;
			text.Size = size;
			text.Color = color;
			text.Layer = layer;

			entities.push_back(entity);
		}

		return entities;
	}
}
