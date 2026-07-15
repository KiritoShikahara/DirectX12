#include "pch.h"
#include "ThreadPool.h"

#include <algorithm>

namespace utility
{
	ThreadPool::~ThreadPool()
	{
		Finalize();
	}

	void ThreadPool::Initialize(size_t threadCount)
	{
		if (mRunning.load(std::memory_order_relaxed))
		{
			return;
		}

		if (threadCount == 0)
		{
			const unsigned hw = std::thread::hardware_concurrency();
			threadCount = (hw > 1) ? static_cast<size_t>(hw - 1) : 1;
		}

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
		if (!mRunning.load(std::memory_order_relaxed))
		{
			return;
		}

		{
			std::lock_guard<std::mutex> lock(mMutex);
			mRunning.store(false, std::memory_order_relaxed);
		}
		mWakeCv.notify_all();

		for (auto& worker : mWorkers)
		{
			if (worker.joinable())
			{
				worker.join();
			}
		}

		mWorkers.clear();
		mTaskSlots.clear();
	}

	void ThreadPool::Dispatch(std::function<void()>* tasks, size_t count)
	{
		if (count == 0)
		{
			return;
		}

		// count はワーカー数以下であること(呼び出し側の設計契約)
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
		uint64_t lastGeneration = 0;

		while (true)
		{
			std::function<void()>* task = nullptr;

			{
				std::unique_lock<std::mutex> lock(mMutex);
				mWakeCv.wait(lock, [this, &lastGeneration]
					{
						return !mRunning.load(std::memory_order_relaxed)
							|| mGeneration.load(std::memory_order_relaxed) != lastGeneration;
					});

				if (!mRunning.load(std::memory_order_relaxed))
				{
					return;
				}

				lastGeneration = mGeneration.load(std::memory_order_relaxed);
				task = mTaskSlots[workerIndex];
			}

			if (task != nullptr)
			{
				(*task)();

				std::lock_guard<std::mutex> lock(mMutex);
				if (mPendingCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					mDoneCv.notify_all();
				}
			}
		}
	}
}
