#include "pch.h"
#include "SceneManager.h"

#include"../Factory/SceneFactory.h"
#include<graphics/Transition/TransitionRenderer.h>
#include<ecs/entity/EntityManager.h>

namespace sys
{
	/// <summary>
	/// 初期シーンを名前で開始する。
	/// SceneFactory に未登録の場合は DefaultScene で開始する。
	/// </summary>
	void SceneManager::Initialize(const std::string& sceneName)
	{
		mCurrentScene = SceneFactory::Get().Create(sceneName);
		mCurrentSceneName = sceneName;
		mCurrentScene->Initialize();
	}

	void SceneManager::Update(float deltaTime)
	{
		switch (mTransitionState)
		{
		case eTransitionState::FadeOut:
		{
			mFadeAlpha += mFadeSpeed * deltaTime;
			if (mFadeAlpha >= 1.0f)
			{
				mFadeAlpha = 1.0f;
				// フェードアウト完了 → PostUpdate での切り替えを待つ
				mTransitionState = eTransitionState::WaitSwitch;
			}
			break;
		}

		case eTransitionState::FadeIn:
		{
			mFadeAlpha -= mFadeSpeed * deltaTime;
			if (mFadeAlpha <= 0.0f)
			{
				mFadeAlpha = 0.0f;
				mTransitionState = eTransitionState::Idle;
			}
			break;
		}

		case eTransitionState::WaitSwitch:
		case eTransitionState::Idle:
		default:
			break;
		}
	}

	/// <summary>
	/// フラグが立っていたらシーンを切り替える
	/// フレーム末で呼び出すこと。
	/// </summary>
	void SceneManager::PostUpdate()
	{
		// トランジションなし：即切り替え
		if (!mUseTransition && mPendingSceneFactory)
		{
			ApplyPendingScene();
			mTransitionState = eTransitionState::Idle;
			return;
		}

		// トランジションあり：FadeOut が完了した WaitSwitch 状態でのみ切り替える
		if (mTransitionState == eTransitionState::WaitSwitch)
		{
			ApplyPendingScene();
			mTransitionState = eTransitionState::FadeIn;
		}
	}

	/// <summary>
	/// トランジション用のフルスクリーンオーバーレイを描画する。
	/// シーンの描画コマンド発行後、EndFrame() 直前に呼ぶ。
	/// </summary>
	void SceneManager::DrawTransition(ID3D12GraphicsCommandList* cmdList)
	{
		if (mTransitionState == eTransitionState::Idle) return;
		if (mFadeAlpha <= 0.0f) return;

		graphics::Color color(mFadeColorR, mFadeColorG, mFadeColorB, mFadeAlpha);
		graphics::TransitionRenderer::Get().Draw(cmdList, color);
	}

	/// <summary>
	/// 現シーンを終了させてエンジンを終了する。
	/// </summary>
	void SceneManager::Finalize()
	{
		if (mCurrentScene)
		{
			mCurrentScene->Finalize();
			mCurrentScene.reset();
		}
		mPendingSceneFactory = nullptr;
		mTransitionState = eTransitionState::Idle;
	}

	/// <summary>
	/// トランジションなしで即座にシーンを切り替える（名前指定）。
	/// SceneFactory に未登録の場合は DefaultScene に切り替わる。
	/// </summary>
	void SceneManager::ChangeScene(const std::string& sceneName)
	{
		mPendingSceneFactory = [sceneName]()
			{
				return SceneFactory::Get().Create(sceneName);
			};
		mPendingSceneName = sceneName;
		mUseTransition = false;
	}

	/// <summary>
	/// フェードトランジションつきでシーンを切り替える（名前指定）。
	/// SceneFactory に未登録の場合は DefaultScene に切り替わる。
	/// </summary>
	void SceneManager::ChangeSceneWithTransition(const std::string& sceneName, float fadeSpeed, float r, float g, float b)
	{
		if (mTransitionState != eTransitionState::Idle) return;

		mPendingSceneFactory = [sceneName]()
			{
				return SceneFactory::Get().Create(sceneName);
			};
		mPendingSceneName = sceneName;

		mUseTransition = true;
		mFadeSpeed = (fadeSpeed > 0.0f) ? fadeSpeed : 1.0f;
		mFadeColorR = r;
		mFadeColorG = g;
		mFadeColorB = b;
		mFadeAlpha = 0.0f;
		mTransitionState = eTransitionState::FadeOut;
	}

	/// <summary>
	/// シーン切り替え
	/// </summary>
	void SceneManager::ApplyPendingScene()
	{
		if (!mPendingSceneFactory) return;

		// エンティティの破棄
		::ecs::EntityManager::Get().ClearLocalEntities();

		// 旧シーン終了
		if (mCurrentScene)
		{
			mCurrentScene->Finalize();
			mCurrentScene.reset();
		}

		// 新シーン生成 & 開始
		mCurrentScene = mPendingSceneFactory();
		mPendingSceneFactory = nullptr;

		if (mCurrentScene)
		{
			mCurrentScene->Initialize();
		}
	}
}