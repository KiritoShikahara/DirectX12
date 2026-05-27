#pragma once
#include<entt/entt.hpp>

namespace graphics
{
	/// <summary>
	/// アニメーションシステム
	/// </summary>
	class AnimationSystem
	{
	public:
		static void UpdateAnimation(entt::registry& registry, float deltatime);
	};
}