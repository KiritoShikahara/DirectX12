#pragma once

#include<memory>
#include<functional>
#include<string>
#include<unordered_map>
#include<vector>

#include"../IScene.h"
#include<Utility/Singleton/Singleton.hpp>

namespace sys
{
	/// <summary>
	/// Sceneの生成メソッドを管理するクラス
    /// App側でエンジンのInitializeが呼ばれる前に生成メソッドの登録と最初のシーンの登録を済ませておかないといけない
    /// 現状はエディタとデータベースがないためこのような構成にしているが本来は、エンジンの前に設定のコンフィグデータをすべて読み込めるように
	/// </summary>
	class SceneFactory : public utility::Singleton<SceneFactory>
	{
		SINGLETON_CLASS(SceneFactory);
	public:
		SINGLETON_ACCESSOR(SceneFactory);

		using CreatorFunc = std::function<std::unique_ptr<IScene>()>;

        /// <summary>
        /// シーンのクリエータ関数を名前で登録する。
        /// REGISTER_SCENE マクロ経由での使用を推奨。
        /// </summary>
        void Register(const std::string& name, CreatorFunc creator);

        /// <summary>
        /// 名前に対応するシーンを生成して返す。
        /// 未登録の name を渡した場合は DefaultScene を返す。
        /// </summary>
        std::unique_ptr<IScene> Create(const std::string& name) const;

        /// <summary>登録済みシーン名の一覧を返す（デバッグ用）</summary>
        std::vector<std::string> GetRegisteredNames() const;

        /// <summary>指定の name が登録済みかどうかを返す</summary>
        bool IsRegistered(const std::string& name) const;

        /// <summary>app起動時のdefaultのシーン名をセットする</summary>
        void SetDefaultSceneName(const std::string& name);

        /// <summary>app起動時のdefaultのシーン名を取得</summary>
        const std::string& GetDefaultSceneName();

	private:
		std::unordered_map<std::string, CreatorFunc> mCreators;
        std::string mDefaultSceneName;
	};

    /// <summary>
    /// 自動登録ヘルパー
    /// コンストラクタでRegisterを呼ぶだけ
    /// </summary>
    struct SceneAutoRegister
    {
        SceneAutoRegister(const std::string& name, SceneFactory::CreatorFunc creator)
        {
            SceneFactory::Get().Register(name, std::move(creator));
        }
    };

}

/// <summary>
/// クラス名をキーとして SceneFactory に登録する。
///
/// 例:
///   REGISTER_SCENE(GameScene);
/// </summary>
#define REGISTER_SCENE(SceneClass) \
    static ::sys::SceneAutoRegister s_SceneAutoReg_##SceneClass( \
        #SceneClass, \
        []() -> std::unique_ptr<::sys::IScene> \
        { return std::make_unique<SceneClass>(); })

/// <summary>
/// キー名を明示指定して SceneFactory に登録する。
///
/// 例:
///   REGISTER_SCENE_AS(GameScene, "Game");
/// </summary>
#define REGISTER_SCENE_AS(SceneClass, NameStr) \
    static ::sys::SceneAutoRegister s_SceneAutoReg_##SceneClass( \
        NameStr, \
        []() -> std::unique_ptr<::sys::IScene> \
        { return std::make_unique<SceneClass>(); })



