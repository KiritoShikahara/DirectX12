#pragma once

#include <entt/entt.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <Utility/Export/Export.h>
#include <Utility/Singleton/Singleton.hpp>
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
	private:
        /// <summary>
        /// フェーズごとに管理するユーザー定義システムのリスト
        /// </summary>
        std::unordered_map<eUpdatePhase, std::vector<std::unique_ptr<IUserSystem>>> mUserSystems;
	};
}


