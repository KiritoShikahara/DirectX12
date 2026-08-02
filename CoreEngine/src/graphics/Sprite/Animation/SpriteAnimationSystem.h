#pragma once

#include<entt/entt.hpp>

namespace sys
{
	/// <summary>
	/// ecs::SpriteAnimation を持つエンティティを更新し、
	/// 経過時間に応じて ecs::Sprite::UVOffset / UVScale を切り替えるシステム。
	/// SpriteRenderer::UpdateAndDraw() より前の Update フェーズで呼ぶこと。
	/// </summary>
	class SpriteAnimationSystem
	{
	public:
		/// <summary>
		/// SpriteAnimation を持つ全エンティティのコマ送りを進める。
		/// </summary>
		/// <param name="registry">ECS レジストリ</param>
		/// <param name="deltaTime">前フレームからの経過時間（秒）</param>
		static void Update(entt::registry& registry, float deltaTime);
	};
}