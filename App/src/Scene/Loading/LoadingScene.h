#pragma once

#include<system/Scene/IScene.h>
#include<system/Loading/LoadingScreenComponent.h>
#include<functional>
#include<thread>

namespace scene
{
	/// <summary>
	/// 重いアセット読み込みの間だけ表示する軽量なつなぎシーン。
	///
	/// 流れ:
	///   1. Initialize()でローディングUI(スピナー・テキスト)だけを同期的に読み込み・表示する
	///      (ここで使う画像/フォントは既にキャッシュ済みの想定で、実質一瞬で終わる)
	///   2. preloadFnをバックグラウンドスレッドで実行する(例: GameScene::PreloadAssetsCpu、
	///      TextureManager::PreloadBatchDecode/FbxResourceManager::PreloadBatchParse呼び出し)。
	///      preloadFnはCPU専用処理(ファイル解析・画像デコード)のみを行うこと。
	///      entt::registryにもD3D12にも一切触れないこと(GPUリソース生成をここで行うと
	///      メインスレッドの描画と競合しGPU同期を壊す。詳細はLoadingScreenComponent参照)。
	///   3. メインスレッドはLoadingScreenUpdateSystem経由でスピナーを回しつつ、
	///      preloadFnの完了を毎フレームポーリングする
	///   4. 完了を検知した瞬間、resolveFnをメインスレッドから同期的に呼ぶ(GPUリソース生成。
	///      例: GameScene::PreloadAssetsGpu → FbxResourceManager::PreloadBatchResolve)
	///   5. resolveFn完了直後、onCompleteをメインスレッドから呼ぶ(通常はここで
	///      SceneManager::ChangeSceneWithTransition&lt;実際の遷移先&gt;(...)を呼ぶ)
	///
	/// IScene/SceneManagerの既存インターフェースには一切手を入れず、LoadingScene
	/// 自体を「ただの1シーン」として扱えるようにしている(遷移先シーン側の変更も不要)。
	/// </summary>
	class LoadingScene : public ::sys::IScene
	{
	public:
		// preloadFn/resolveFnはどちらも::ecs::LoadingProgress&を渡されて呼ばれる。
		// 読み込み総数が判明した時点でProgress.Totalに加算し、1項目完了するごとに
		// Progress.Loadedをインクリメントすること
		// (FbxResourceManager::PreloadBatchParse/Resolveの
		// onTotalDiscovered/onItemLoadedコールバック引数を参照)。
		//
		// preloadFn: バックグラウンドスレッドで実行される。CPU専用処理のみ許可。
		// resolveFn: preloadFn完了後、メインスレッドで同期的に実行される。GPUリソース生成用。
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
