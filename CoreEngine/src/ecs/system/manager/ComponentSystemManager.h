#pragma once

#include <entt/entt.hpp>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <Utility/Export/Export.h>
#include <Utility/Singleton/Singleton.hpp>
#include <Utility/config/DebugConfig.h>
#include "IComponentSystem.h"

namespace ecs
{
    /// <summary>
    /// システムの更新タイミングを定義するフェーズ
    /// </summary>
    enum class eUpdatePhase
    {
        PreUpdate,
        Update,
        PostUpdate,
    };

    /// <summary>
    /// ユーザー定義システムとエンジン固定システムを統括するクラス
    /// </summary>
    class ComponentSystemManager : public utility::Singleton<ComponentSystemManager>
    {
        SINGLETON_CLASS(ComponentSystemManager);
    public:
        SINGLETON_ACCESSOR(ComponentSystemManager);

        /// <summary>
        /// ユーザー定義システムを指定フェーズに登録する
        /// </summary>
        template <typename T, typename... Args>
        void AddUserSystem(eUpdatePhase phase, Args&&... args)
        {
            static_assert(std::is_base_of_v<IUserSystem, T>, "T must inherit from sys::IUserSystem");
            mUserSystems[phase].push_back(std::make_unique<T>(std::forward<Args>(args)...));
        }

        /// <summary>
        /// 指定フェーズのシステムを登録順に一括実行する
        /// </summary>
        void ExecutePhase(eUpdatePhase phase, entt::registry& registry, float deltaTime, float rawDeltaTime);

        /// <summary>
        /// 登録済みの全ユーザーシステムを削除する
        /// </summary>
        void ClearUserSystems();

        /// <summary>
        /// System1つ分の直近の所要時間
        /// </summary>
        struct SystemTiming
        {
            std::string Name;
            float       SmoothedMs = 0.0f;
        };

        /// <summary>
        /// 指定フェーズに登録されている各Systemの直近の所要時間を返す
        /// </summary>
        const std::vector<SystemTiming>& GetSystemTimings(eUpdatePhase phase) const;

    private:
        /// <summary>
        /// フェーズごとに管理するユーザー定義システムのリスト
        /// </summary>
        std::unordered_map<eUpdatePhase, std::vector<std::unique_ptr<IUserSystem>>> mUserSystems;

#if DEV_TOOL_ENABLED
        /// <summary>
        /// フェーズごとのSystem所要時間計測値
        /// </summary>
        std::unordered_map<eUpdatePhase, std::vector<SystemTiming>> mSystemTimings;
#endif
    };
}