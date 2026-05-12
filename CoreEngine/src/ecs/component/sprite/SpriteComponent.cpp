#include "SpriteComponent.h"
#include<graphics/Texture/Texture.h>

/// <summary>
/// コンストラクタ。テクスチャの実サイズを取得したいのと依存注入
/// </summary>
ecs::Sprite::Sprite(graphics::Texture* texture)
{
	if (texture)
	{
		Texture = texture;
		Size = { texture->GetWidth(),texture->GetHeight() };
	}
}

/// <summary>
/// レイヤー設定用のヘルパー
/// </summary>
/// <param name="base"></param>
/// <param name="offset"></param>
void ecs::Sprite::SetLayer(SpriteLayer base, int offset)
{
	Layer = static_cast<int>(base) + static_cast<int>(offset);
}
