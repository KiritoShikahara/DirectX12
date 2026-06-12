#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include"../IScene.h"

#include<concepts>
#include<functional>
#include<memory>

struct ID3D12GraphicsCommandList;

namespace sys
{

	/// <summary>
	/// トランジションの状態
	/// </summary>
	enum class eTransitionState
	{
		Idle,       // トランジションなし（通常状態）
		FadeOut,    // 現シーンをフェードアウト中
		WaitSwitch,     // フェードアウト完了、PostUpdate でシーン切り替え待ち
		FadeIn,     // 次シーンをフェードイン中
	};

	// Scene以外から呼ばれたくないので。
	template<typename T>
	concept TScene = std::derived_from<T, IScene>;

	/// <summary>
	/// シーン管理
	/// </summary>
	class SceneManager : public utility::Singleton<SceneManager>
	{
		SINGLETON_CLASS(SceneManager);
	public:
		SINGLETON_ACCESSOR(SceneManager);

		template<TScene T, typename... Args>
		void Initialize(Args&&... args);


		/// <summary>
		/// 初期シーンを名前で開始する。
		/// SceneFactory に未登録の場合は DefaultScene で開始する。
		/// </summary>
		void Initialize(const std::string& sceneName);

		/// <summary>
		/// シーン切り替えリクエスト・トランジション進行を処理する。
		/// メインループの Update 相当フェーズで毎フレーム呼ぶ。
		/// </summary>
		/// <param name="deltaTime">前フレームからの経過秒数</param>
		void Update(float deltaTime);

		/// <summary>
		/// フラグが立っていたらシーンを切り替える
		/// フレーム末で呼び出すこと。
		/// </summary>
		void PostUpdate();

		/// <summary>
		/// トランジション用のフルスクリーンオーバーレイを描画する。
		/// シーンの描画コマンド発行後、EndFrame() 直前に呼ぶ。
		/// </summary>
		void DrawTransition(ID3D12GraphicsCommandList* cmdList);

		/// <summary>
		/// 現シーンを終了させてエンジンを終了する。
		/// </summary>
		void Finalize();

		/// <summary>
		/// トランジションなしで即座にシーンを切り替える。
		/// 現フレーム末尾（次 Update() 冒頭）に切り替えが行われる。
		/// </summary>
		template<TScene T, typename... Args>
		void ChangeScene(Args&&... args);

		/// <summary>
		/// トランジションなしで即座にシーンを切り替える（名前指定）。
		/// SceneFactory に未登録の場合は DefaultScene に切り替わる。
		/// </summary>
		void ChangeScene(const std::string& sceneName);

		/// <summary>
		/// フェードトランジションつきでシーンを切り替える。
		/// FadeOut → シーン切り替え → FadeIn の順に自動進行する。
		/// </summary>
		/// <param name="fadeSpeed">フェード速度（alpha/秒）。デフォルト 1.0f = 1 秒でフェード完了</param>
		/// <param name="r">フェード色 R [0,1]</param>
		/// <param name="g">フェード色 G [0,1]</param>
		/// <param name="b">フェード色 B [0,1]</param>
		template<TScene T, typename... Args>
		void ChangeSceneWithTransition(
			float fadeSpeed = 1.0f,
			float r = 0.0f, float g = 0.0f, float b = 0.0f,
			Args&&... args);

		/// <summary>
		/// フェードトランジションつきでシーンを切り替える（名前指定）。
		/// SceneFactory に未登録の場合は DefaultScene に切り替わる。
		/// </summary>
		void ChangeSceneWithTransition(
			const std::string& sceneName,
			float fadeSpeed = 1.0f,
			float r = 0.0f, float g = 0.0f, float b = 0.0f);
	private:
		/// <summary>
		/// シーン切り替え
		/// </summary>
		void ApplyPendingScene();

	private:

		/// <summary>
		/// 今のシーン
		/// </summary>
		std::unique_ptr<IScene> mCurrentScene;

		/// <summary>
		/// 次に切り替えるSceneファクトリ nullptr == 切り替えなし
		/// </summary>
		std::function<std::unique_ptr<IScene>()> mPendingSceneFactory;

		/// <summary>
		/// 今のスクリーンの名前
		/// </summary>
		std::string mCurrentSceneName;

		/// <summary>
		/// 次に切り替えるスクリーンの名前
		/// </summary>
		std::string mPendingSceneName;

		/// <summary>
		/// トランジションの状態
		/// </summary>
		eTransitionState mTransitionState = eTransitionState::Idle;

		float mFadeAlpha = 0.0f; // 現在のフェード不透明度 [0,1]
		float mFadeSpeed = 1.0f; // フェード速度（alpha/秒）
		float mFadeColorR = 0.0f;
		float mFadeColorG = 0.0f;
		float mFadeColorB = 0.0f;

		bool mUseTransition = false; // 今回の切り替えにトランジションを使うか
	};

	template<TScene T, typename ...Args>
	inline void SceneManager::Initialize(Args && ...args)
	{
		mCurrentScene = std::make_unique<T>(std::forward<Args>(args)...);
		mCurrentScene->Initialize();
	}

	template<TScene T, typename ...Args>
	inline void SceneManager::ChangeScene(Args && ...args)
	{
		mPendingSceneFactory = [args = std::make_tuple(std::forward<Args>(args)...)]() mutable
			{
				return std::apply(
					[](auto&&... a) { return std::make_unique<T>(std::forward<decltype(a)>(a)...); },
					std::move(args));
			};
		mUseTransition = false;
	}

	template<TScene T, typename ...Args>
	inline void SceneManager::ChangeSceneWithTransition(float fadeSpeed, float r, float g, float b, Args && ...args)
	{
		// すでにトランジション中なら無視
		if (mTransitionState != eTransitionState::Idle) return;

		mPendingSceneFactory = [args = std::make_tuple(std::forward<Args>(args)...)]() mutable
			{
				return std::apply(
					[](auto&&... a) { return std::make_unique<T>(std::forward<decltype(a)>(a)...); },
					std::move(args));
			};

		mUseTransition = true;
		mFadeSpeed = (fadeSpeed > 0.0f) ? fadeSpeed : 1.0f;
		mFadeColorR = r;
		mFadeColorG = g;
		mFadeColorB = b;
		mFadeAlpha = 0.0f;
		mTransitionState = eTransitionState::FadeOut;
	}


}


