#include"pch.h"
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

/// <summary>
/// スプライトシートの行列数を指定して、UVScale を一括設定するヘルパー。
/// </summary>
void ecs::Sprite::SetSheetGrid(int columns, int rows)
{
	if (columns <= 0 || rows <= 0) return;

	UVScale = { 1.0f / static_cast<float>(columns), 1.0f / static_cast<float>(rows) };
}

/// <summary>
/// 0始まりのフレーム番号から UVOffset を計算して設定するヘルパー。
/// </summary>
void ecs::Sprite::SetFrame(int frameIndex, int columns)
{
	if (columns <= 0 || frameIndex < 0) return;

	const int col = frameIndex % columns;
	const int row = frameIndex / columns;

	UVOffset = { static_cast<float>(col) * UVScale.x, static_cast<float>(row) * UVScale.y };
}