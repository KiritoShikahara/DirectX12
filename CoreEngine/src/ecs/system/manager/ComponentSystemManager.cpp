#include "pch.h"
#include "ComponentSystemManager.h"

#if defined(_DEBUG) || defined(DEV_TOOL_ENABLED)
#include <chrono>
#include <typeinfo>
#endif

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

#if defined(_DEBUG) || defined(DEV_TOOL_ENABLED)
		// System単位の所要時間を計測する(Performanceデバッグウィンドウでの内訳表示用)。
		// 登録済みSystem数が変わった場合(シーン切り替え等)のみ計測用配列を作り直す
		auto& timings = mSystemTimings[phase];
		if (timings.size() != it->second.size())
		{
			timings.clear();
			timings.reserve(it->second.size());
			for (const auto& system : it->second)
			{
				timings.push_back({ typeid(*system).name(), 0.0f });
			}
		}

		for (size_t i = 0; i < it->second.size(); ++i)
		{
			const auto start = std::chrono::high_resolution_clock::now();
			it->second[i]->Update(registry, deltaTime, rawDeltaTime);
			const auto end = std::chrono::high_resolution_clock::now();
			const float ms = std::chrono::duration<float, std::milli>(end - start).count();

			// 指数移動平均で平滑化する(毎フレーム更新すると数値が激しく点滅して読みづらいため)
			timings[i].SmoothedMs = timings[i].SmoothedMs * 0.9f + ms * 0.1f;
		}
#else
		for (const auto& system : it->second)
		{
			system->Update(registry, deltaTime, rawDeltaTime);
		}
#endif
	}

	void ComponentSystemManager::ClearUserSystems()
	{
		mUserSystems.clear();
#if defined(_DEBUG) || defined(DEV_TOOL_ENABLED)
		mSystemTimings.clear();
#endif
	}

	const std::vector<ComponentSystemManager::SystemTiming>& ComponentSystemManager::GetSystemTimings(eUpdatePhase phase) const
	{
#if defined(_DEBUG) || defined(DEV_TOOL_ENABLED)
		const auto it = mSystemTimings.find(phase);
		if (it != mSystemTimings.end())
		{
			return it->second;
		}
#endif
		static const std::vector<SystemTiming> empty;
		return empty;
	}
}