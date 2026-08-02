#include "pch.h"
#include "ThreadPool.h"

#include <algorithm>
#include <combaseapi.h>

namespace utility
{
	ThreadPool::~ThreadPool()
	{
		Finalize();
	}

	void ThreadPool::Initialize(size_t threadCount)
	{
		// 初期化済みなら処理終了
		if (mRunning.load(std::memory_order_relaxed))
		{
			return;
		}

		// カウント数が０より小さいならCPUのスレッド数ー１
		if (threadCount <= 0)
		{
			const unsigned hw = std::thread::hardware_concurrency();
			threadCount = (hw > 1) ? static_cast<size_t>(hw - 1) : 1;
		}

		// 初期化
		mTaskSlots.assign(threadCount, nullptr);
		mRunning.store(true, std::memory_order_relaxed);

		mWorkers.reserve(threadCount);
		for (size_t i = 0; i < threadCount; ++i)
		{
			mWorkers.emplace_back(&ThreadPool::WorkerLoop, this, i);
		}
	}

	void ThreadPool::Finalize()
	{
		// 未読み込みなら終了
		if (!mRunning.load(std::memory_order_relaxed))
		{
			return;
		}

		{
			std::lock_guard<std::mutex> lock(mMutex);
			mRunning.store(false, std::memory_order_relaxed);
		}
		mWakeCv.notify_all();

		// スレッドの同期待ちをしてから
		for (auto& worker : mWorkers)
		{
			if (worker.joinable())
			{
				worker.join();
			}
		}

		// タスクの削除とリソース開放
		mWorkers.clear();
		mTaskSlots.clear();
	}

	void ThreadPool::Dispatch(std::function<void()>* tasks, size_t count)
	{
		if (count == 0)
		{
			return;
		}

		// count はワーカー数以下であること
		count = std::min(count, mTaskSlots.size());

		{
			std::lock_guard<std::mutex> lock(mMutex);
			for (size_t i = 0; i < mTaskSlots.size(); ++i)
			{
				mTaskSlots[i] = (i < count) ? &tasks[i] : nullptr;
			}
			mPendingCount.store(count, std::memory_order_relaxed);
			mGeneration.fetch_add(1, std::memory_order_relaxed);
		}
		mWakeCv.notify_all();
	}

	void ThreadPool::WaitAll()
	{
		std::unique_lock<std::mutex> lock(mMutex);
		mDoneCv.wait(lock, [this]
			{
				return mPendingCount.load(std::memory_order_relaxed) == 0;
			});
	}

	void ThreadPool::WorkerLoop(size_t workerIndex)
	{
		// このワーカースレッド専用にCOMを初期化
		const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool comInitialized = SUCCEEDED(comHr);

		// 最終の世代数保存用
		uint64_t lastGeneration = 0;

		while (true)
		{
			std::function<void()>* task = nullptr;

			{
				std::unique_lock<std::mutex> lock(mMutex);
				// 終了フラグが立つか、新しい世代のタスクが通知されるまで待機
				mWakeCv.wait(lock, [this, &lastGeneration]
					{
						return !mRunning.load(std::memory_order_relaxed)
							|| mGeneration.load(std::memory_order_relaxed) != lastGeneration;
					});

				// スレッドプールの終了要請があったら解放
				if (!mRunning.load(std::memory_order_relaxed))
				{
					if (comInitialized) CoUninitialize();
					return;
				}

				// 現在の世代更新をして自身のスレッドインデックスを割り当て
				lastGeneration = mGeneration.load(std::memory_order_relaxed);
				task = mTaskSlots[workerIndex];
			}

			// 割り当てたタスクが存在する場合は実行する
			if (task != nullptr)
			{
				(*task)();

				// タスクの完了通知
				std::lock_guard<std::mutex> lock(mMutex);
				if (mPendingCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					mDoneCv.notify_all();
				}
			}
		}
	}
}
