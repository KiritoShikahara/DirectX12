#include"pch.h"
#include "SpriteAnimationSystem.h"

#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/sprite/SpriteAnimationComponent.h>

namespace sys
{
	/// <summary>
	/// SpriteAnimation を持つ全エンティティのコマ送りを進める。
	/// </summary>
	void SpriteAnimationSystem::Update(entt::registry& registry, float deltaTime)
	{
		auto view = registry.view<ecs::Sprite, ecs::SpriteAnimation>();

		view.each([deltaTime](auto, ecs::Sprite& sprite, ecs::SpriteAnimation& anim)
			{
				if (!anim.IsPlaying) return;
				if (anim.Columns <= 0 || anim.Rows <= 0) return;

				const int totalFrames = anim.Columns * anim.Rows;
				const int frameCount = (anim.FrameCount > 0)
					? std::min(anim.FrameCount, totalFrames)
					: totalFrames;

				if (frameCount <= 0) return;

				// 経過時間を加算し、FrameDuration を超えた分だけコマを進める
				anim.ElapsedTime += deltaTime;

				while (anim.ElapsedTime >= anim.FrameDuration && anim.FrameDuration > 0.0f)
				{
					anim.ElapsedTime -= anim.FrameDuration;
					anim.CurrentFrame++;

					if (anim.CurrentFrame >= frameCount)
					{
						if (anim.IsLooping)
						{
							anim.CurrentFrame = 0;
						}
						else
						{
							anim.CurrentFrame = frameCount - 1;
							anim.IsPlaying = false;
							break;
						}
					}
				}

				// UVScale はシート構成から固定で決まる
				sprite.UVScale = {
					1.0f / static_cast<float>(anim.Columns),
					1.0f / static_cast<float>(anim.Rows)
				};

				// 実際に切り出すコマ番号（シート全体に対する絶対インデックス）
				const int absoluteFrame = anim.StartFrame + anim.CurrentFrame;
				const int col = absoluteFrame % anim.Columns;
				const int row = absoluteFrame / anim.Columns;

				sprite.UVOffset = {
					static_cast<float>(col) * sprite.UVScale.x,
					static_cast<float>(row) * sprite.UVScale.y
				};
			});
	}
}