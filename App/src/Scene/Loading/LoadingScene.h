#pragma once

#include<system/Scene/IScene.h>
#include<system/Loading/LoadingScreenComponent.h>
#include<functional>
#include<thread>

namespace scene
{
	///<summary>
	///重いアセット読み込み中だけ表示するローディングシーン。preloadFnを別スレッドで実行し、完了後にresolveFnとonCompleteをメインスレッドから呼ぶ
	///</summary>
	class LoadingScene : public ::sys::IScene
	{
	public:
		///<summary>
		///preloadFnはバックグラウンドスレッドで実行するCPU専用処理、resolveFnは完了後にメインスレッドで呼ぶGPUリソース生成処理
		///</summary>
		LoadingScene(
			std::function<void(::ecs::LoadingProgress&)> preloadFn,
			std::function<void(::ecs::LoadingProgress&)> resolveFn,
			std::function<void()> onComplete);

		virtual void Initialize() override;
		virtual void Finalize() override;

	private:
		void CreateLoadingUi();

		std::function<void(::ecs::LoadingProgress&)> mPreloadFn;
		std::function<void(::ecs::LoadingProgress&)> mResolveFn;
		std::function<void()> mOnComplete;
		std::thread mLoadingThread;
	};
}
