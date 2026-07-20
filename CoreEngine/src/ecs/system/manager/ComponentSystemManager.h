#pragma once

#include <entt/entt.hpp>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <Utility/Export/Export.h>
#include <Utility/Singleton/Singleton.hpp>
// mSystemTimingsの有無をDEV_TOOL_ENABLEDで切り替えるため、このヘッダをincludeする
// 全翻訳単位で同じ値が見えるようDebugConfig.hを直接includeする(pch.h任せにすると
// pch.hを使わないApp側とクラスのサイズが食い違いODR違反になる)
#include <Utility/config/DebugConfig.h>
#include "IComponentSystem.h"

namespace ecs
{
    /// <summary>
    /// システムの更新タイミングを定義するフェーズ
    /// </summary>
    enum class eUpdatePhase
    {
        PreUpdate,   // 物理・メイン更新の前処理
        Update,      // メインのゲームロジック
        PostUpdate,  // 後処理
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
        /// 登録順に実行されるため、依存関係がある場合は呼び出し順を考慮すること
        /// </summary>
        /// <typeparam name="T">IUserSystem を継承したシステムクラス</typeparam>
        /// <typeparam name="Args">コンストラクタ引数の型</typeparam>
        /// <param name="phase">実行タイミング</param>
        /// <param name="args">コンストラクタ引数</param>
        template <typename T, typename... Args>
        void AddUserSystem(eUpdatePhase phase, Args&&... args)
        {
            static_assert(
                std::is_base_of_v<IUserSystem, T>,
                "T must inherit from sys::IUserSystem"
                );
            mUserSystems[phase].push_back(
                std::make_unique<T>(std::forward<Args>(args)...)
            );
        }

        /// <summary>
        /// 指定フェーズのシステムを登録順に一括実行する
        /// FixedUpdate フェーズはエンジン側の物理ステップも内部で実行される
        /// </summary>
        /// <param name="phase">実行するフェーズ</param>
        /// <param name="registry">ECSレジストリ</param>
        /// <param name="deltaTime">経過時間(秒)</param>
        void ExecutePhase(eUpdatePhase phase, entt::registry& registry, float deltaTime,float rawDeltaTime);

        /// <summary>
        /// 登録済みの全ユーザーシステムを削除する
        /// </summary>
        void ClearUserSystems();

        /// <summary>
        /// System1つ分の直近の所要時間(指数移動平均で平滑化済み)
        /// </summary>
        struct SystemTiming
        {
            std::string Name;
            float       SmoothedMs = 0.0f;
        };

        /// <summary>
        /// 指定フェーズに登録されている各Systemの直近の所要時間を返す(登録順)。
        /// _DEBUG または DEV_TOOL_ENABLED 時のみ計測しており、それ以外では常に空を返す
        /// (Releaseビルドで計測コストを払わないため)。
        /// </summary>
        const std::vector<SystemTiming>& GetSystemTimings(eUpdatePhase phase) const;

	private:
        /// <summary>
        /// フェーズごとに管理するユーザー定義システムのリスト
        /// </summary>
        std::unordered_map<eUpdatePhase, std::vector<std::unique_ptr<IUserSystem>>> mUserSystems;

#if DEV_TOOL_ENABLED
        // フェーズごとのSystem所要時間計測値(mUserSystemsと同じ順序で対応する)
        std::unordered_map<eUpdatePhase, std::vector<SystemTiming>> mSystemTimings;
#endif
	};
}


