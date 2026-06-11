#include "pch.h"
#include "SceneManager.h"

#include<graphics/Transition/TransitionRenderer.h>

namespace sys
{
	void SceneManager::Update(float deltaTime)
	{
		// トランジションなし即時切替
		if (mUseTransition && mPendingSceneFactory)
		{
			this->ApplyPendingScene();
			return;
		}

		// トランジションあり
		switch (mTransitionState)
		{
		case sys::eTransitionState::FadeOut:
			mFadeAlpha += mFadeSpeed * deltaTime;
			if (mFadeAlpha >= 1.0f)
			{
				mFadeAlpha = 1.0f;
				// フェードアウト完了 → シーン切り替え
				ApplyPendingScene();
				mTransitionState = eTransitionState::FadeIn;
			}
			break;
		case sys::eTransitionState::FadeIn:
			mFadeAlpha -= mFadeSpeed * deltaTime;
			if (mFadeAlpha <= 0.0f)
			{
				mFadeAlpha = 0.0f;
				mTransitionState = eTransitionState::Idle;
				mUseTransition = false;
			}
			break;
		case sys::eTransitionState::Idle:
		default:
			break;
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
	/// シーン切り替え
	/// </summary>
	void SceneManager::ApplyPendingScene()
	{
		if (!mPendingSceneFactory) return;

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