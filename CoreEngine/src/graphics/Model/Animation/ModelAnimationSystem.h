#pragma once

#include<entt/entt.hpp>

namespace graphics
{
	class ModelAnimationSystem
	{
	public:
		/// <summary>
		/// Model + ModelAnimComponent を持つ全エンティティのアニメーション時間を進める
		/// IsPlaying が true のエンティティのみ更新する
		/// </summary>
		static void Update(entt::registry& registry, float deltaTime);

	};
}


