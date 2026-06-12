#include "pch.h"
#include "ComponentSystemManager.h"

namespace ecs
{
	void ComponentSystemManager::ExecutePhase(eUpdatePhase phase, entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// ユーザー定義システムの実行
		const auto it = mUserSystems.find(phase);
		if (it == mUserSystems.end())
		{
			return;
		}

		for (const auto& system : it->second)
		{
			system->Update(registry, deltaTime, rawDeltaTime);
		}
	}

	void ComponentSystemManager::ClearUserSystems()
	{
		mUserSystems.clear();
	}
}