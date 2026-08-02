#include "pch.h"
#include "ComponentSystemManager.h"
#if DEV_TOOL_ENABLED
#include <chrono>
#include <typeinfo>
#endif

namespace ecs
{
    // 指定フェーズのシステムを登録順に一括実行する
    void ComponentSystemManager::ExecutePhase(eUpdatePhase phase, entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        const auto it = mUserSystems.find(phase);
        if (it == mUserSystems.end())
        {
            return;
        }

#if DEV_TOOL_ENABLED
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

            timings[i].SmoothedMs = timings[i].SmoothedMs * 0.9f + ms * 0.1f;
        }
#else
        for (const auto& system : it->second)
        {
            system->Update(registry, deltaTime, rawDeltaTime);
        }
#endif
    }

    // 登録済みの全ユーザーシステムを削除する
    void ComponentSystemManager::ClearUserSystems()
    {
        mUserSystems.clear();
#if DEV_TOOL_ENABLED
        mSystemTimings.clear();
#endif
    }

    // 指定フェーズのシステム所要時間を取得する
    const std::vector<ComponentSystemManager::SystemTiming>& ComponentSystemManager::GetSystemTimings(eUpdatePhase phase) const
    {
#if DEV_TOOL_ENABLED
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