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
	/// フォークジョイン専用の固定スレッド数ワーカープール。
	///
	/// 汎用タスクキューは持たず、Dispatch() で渡した最大 threadCount 個の
	/// タスクをワーカーへ1対1で割り当てる方式のため、Dispatch自体は
	/// ヒープ確保を行わない。毎フレーム呼び出す用途(マルチスレッドレンダリング等)
	/// を想定しており、ワーカースレッド自体は Initialize() で一度だけ生成する。
	/// </summary>
	class ENGINE_API ThreadPool
	{
	public:
		ThreadPool() = default;
		~ThreadPool();

		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;

		/// <summary>
		/// ワーカースレッドを起動する。
		/// </summary>
		/// <param name="threadCount">ワーカー数(0の場合はハードウェアコア数-1、最低1)</param>
		void Initialize(size_t threadCount = 0);

		/// <summary>
		/// 全ワーカースレッドを停止・終了する。
		/// </summary>
		void Finalize();

		/// <summary>
		/// tasks[0..count) を各ワーカーへ1つずつ割り当てて非同期実行を開始する。
		/// count はワーカー数以下であること。
		/// tasks の指す実体は WaitAll() が返るまで呼び出し側が生存させること。
		/// 直前の Dispatch() の WaitAll() が完了する前に Dispatch() を再度呼ばないこと
		/// (フォークジョイン専用であり、多重発行はサポートしない)。
		/// </summary>
		void Dispatch(std::function<void()>* tasks, size_t count);

		/// <summary>
		/// 直近の Dispatch() で発行した全タスクの完了を待つ。
		/// </summary>
		void WaitAll();

		/// <summary>ワーカースレッド数を返す(Dispatch()に渡せる最大タスク数)</summary>
		size_t WorkerCount() const { return mTaskSlots.size(); }

	private:
		void WorkerLoop(size_t workerIndex);

		std::vector<std::thread> mWorkers;

		/// <summary>ワーカーごとに割り当てられた実行中タスクへのポインタ(nullptr=担当なし)</summary>
		std::vector<std::function<void()>*> mTaskSlots;

		std::atomic<bool>   mRunning{ false };

		/// <summary>Dispatch() の度にインクリメントし、ワーカーへ新規タスク投入を知らせる世代カウンタ</summary>
		std::atomic<uint64_t> mGeneration{ 0 };

		/// <summary>未完了タスク数。0になったらWaitAll()側を起床させる</summary>
		std::atomic<size_t> mPendingCount{ 0 };

		std::mutex              mMutex;
		std::condition_variable mWakeCv;
		std::condition_variable mDoneCv;
	};
}
