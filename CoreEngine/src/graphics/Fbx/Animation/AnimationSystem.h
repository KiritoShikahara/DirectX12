#pragma once

namespace ecs
{
	class EntityManager;
}

namespace graphics
{
	/// <summary>
	/// アニメーションシステム
	/// </summary>
	class AnimationSystem
	{
	public:
		static void UpdateAnimation(ecs::EntityManager& entityManager, float deltaTime);
	};
}