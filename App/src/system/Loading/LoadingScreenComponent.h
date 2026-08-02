#pragma once

#include<entt/entt.hpp>
#include<atomic>
#include<functional>
#include<memory>

namespace ecs
{
	///<summary>
	///先読み進捗のカウンタ。進捗報告はstd::functionコールバック経由で行い、Totalは途中で増えることがあるため0の間は0%として扱う
	///</summary>
	struct LoadingProgress
	{
		std::atomic<int> Loaded{ 0 };
		std::atomic<int> Total{ 0 };
	};

	///<summary>
	///LoadingSceneが生成するコントローラーエンティティに付与するコンポーネント。CPU処理とGPU処理をスレッド分離し、IsCpuDone検知後にResolveFnとOnCompleteをメインスレッドで順に呼ぶ
	///</summary>
	struct LoadingScreenComponent
	{
		std::shared_ptr<std::atomic<bool>> IsCpuDone = std::make_shared<std::atomic<bool>>(false);

		///<summary>
		///バックグラウンドスレッドとLoadingScreenUpdateSystemが共有する進捗カウンタ
		///</summary>
		std::shared_ptr<LoadingProgress> Progress = std::make_shared<LoadingProgress>();

		///<summary>
		///GPUリソース生成フェーズ。メインスレッド専用、IsCpuDone検知後に1回だけ呼ばれる
		///</summary>
		std::function<void(LoadingProgress&)> ResolveFn;

		///<summary>
		///ResolveFn完了直後、メインスレッドから1回だけ呼ばれる
		///</summary>
		std::function<void()> OnComplete;

		///<summary>
		///ResolveFn/OnCompleteを呼び出し済みかどうか。二重呼び出し防止
		///</summary>
		bool Resolved = false;
	};

	///<summary>
	///ローディング画面のスピナー、回転アイコンSpriteに付与するタグ。LoadingScreenUpdateSystemが毎フレームSet2DRotationを進める
	///</summary>
	struct LoadingSpinnerUiTag
	{
		float RotationRad = 0.0f;
	};

	///<summary>
	///進捗バーの塗り部分Spriteに付与するタグ。LoadingScreenUpdateSystemが毎フレームFillAmountへ進捗率を書き込む
	///</summary>
	struct LoadingProgressBarFillTag {};

	///<summary>
	///進捗パーセント表示のTextComponentに付与するタグ
	///</summary>
	struct LoadingProgressTextTag {};
}
