#pragma once

#include <Utility/Export/Export.h>

#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace utility
{
	/// <summary>
	/// フォークジョイン専用の固定スレッド数ワーカープール
	/// </summary>
	class ENGINE_API ThreadPool
	{
	public:
		ThreadPool() = default;
		~ThreadPool();

		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;

		/// <summary>
		/// ワーカースレッドの起動
		/// </summary>
		/// <param name="threadCount">ワーカー数：０の場合はハードウェア数ー１、最低１</param>
		void Initialize(size_t threadCount = 0);

		/// <summary>
		/// 全ワーカースレッドを停止・終了する。
		/// </summary>
		void Finalize();

		/// <summary>
		/// タスクを各ワーカーに割り当てて非同期実行を開始する。
		/// countはワーカー数以下になるように。
		/// </summary>
		void Dispatch(std::function<void()>* tasks, size_t count);

		/// <summary>
		/// 直近の Dispatch() で発行した全タスクの完了を待つ。
		/// </summary>
		void WaitAll();

		/// <summary>
		/// ワーカースレッド数を返す
		/// </summary>
		/// <returns></returns>
		size_t WorkerCount() const { return mTaskSlots.size(); }

	private:
		// 処理ループ
		void WorkerLoop(size_t workerIndex);

		// 各ワーカースレッド
		std::vector<std::thread> mWorkers;

		// ワーカーごとに割り当てた実行中タスクのポインタ
		std::vector<std::function<void()>*> mTaskSlots;

		// 実行中判定用
		std::atomic<bool>   mRunning{ false };

		// 新規タスクの世代カウンタ
		std::atomic<uint64_t> mGeneration{ 0 };

		/// <summary>未完了タスク数。0になったらWaitAll()側を起床させる</summary>
		std::atomic<size_t> mPendingCount{ 0 };

		std::mutex              mMutex;
		std::condition_variable mWakeCv;
		std::condition_variable mDoneCv;
	};
}
