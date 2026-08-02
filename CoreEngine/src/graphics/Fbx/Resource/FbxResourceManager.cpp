#include "pch.h"
#include "FbxResourceManager.h"
#include "FbxResource.h"
#include <graphics/Texture/TextureManager.h>
#include <functional>
#include <algorithm>

namespace graphics
{
	// FBX モデルの読み込み
	FbxResource* FbxResourceManager::Load(const std::string& binPath)
	{
		// キャッシュ済みならそのまま返す
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto it = mCache.find(binPath);
			if (it != mCache.end()) return it->second.get();
		}

		// 読み込み処理
		auto resource = std::make_unique<FbxResource>();
		if (!resource->Load(binPath))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				std::format("FbxResourceManager: Failed to load '{}'", binPath));
			return nullptr;
		}

		FbxResource* ptr = nullptr;

		// キャッシュへの登録
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto it = mCache.emplace(binPath, std::move(resource)).first;
			ptr = it->second.get();
		}
		DEBUG_LOG(sys::eLogLevel::Log,
			std::format("FbxResourceManager: Cached '{}'", binPath));

		return ptr;
	}

	// アニメーションデータの読み込み
	bool FbxResourceManager::LoadAnm(
		const std::string& binPath,
		const std::string& anmPath,
		const std::string& clipName)
	{
		FbxResource* target = nullptr;
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto it = mCache.find(binPath);
			if (it == mCache.end()) return false;
			target = it->second.get();
		}

		return target->LoadAnm(anmPath, clipName);
	}

	// リソースの取得
	FbxResource* FbxResourceManager::GetResource(const std::string& binPath) const
	{
		std::lock_guard<std::mutex> lock(mMutex);
		auto it = mCache.find(binPath);
		return (it != mCache.end()) ? it->second.get() : nullptr;
	}

	// リソースの解放
	void FbxResourceManager::Unload(const std::string& binPath)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		auto it = mCache.find(binPath);
		if (it == mCache.end()) return;
		mCache.erase(it);
	}

	// 全キャッシュのクリア
	void FbxResourceManager::Clear()
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mCache.clear();
		DEBUG_LOG(sys::eLogLevel::Log, "FbxResourceManager: Cleared all cache.");
	}

	// 多数のモデルを事前ロード
	void FbxResourceManager::PreloadBatchParse(const std::vector<std::string>& binPaths,
		const std::function<void(int)>& onTotalDiscovered)
	{
		if (binPaths.empty()) return;

		if (!mLoadThreadPoolStarted)
		{
			mLoadThreadPool.Initialize();
			mLoadThreadPoolStarted = true;
		}

		std::vector<PendingLoad> pending;
		pending.reserve(binPaths.size());
		{
			std::lock_guard<std::mutex> lock(mMutex);
			for (const auto& binPath : binPaths)
			{
				if (mCache.find(binPath) != mCache.end()) continue;
				pending.push_back({ binPath, {} });
			}
		}
		if (pending.empty()) return;

		if (onTotalDiscovered) onTotalDiscovered(static_cast<int>(pending.size()));
		DEBUG_LOG(sys::eLogLevel::Log,
			std::format("FbxResourceManager::PreloadBatchParse: parsing {} model(s)...", pending.size()));

		const size_t workerCount = std::max<size_t>(mLoadThreadPool.WorkerCount(), 1);
		std::vector<std::function<void()>> tasks(workerCount);

		for (size_t offset = 0; offset < pending.size(); offset += workerCount)
		{
			const size_t chunk = std::min(workerCount, pending.size() - offset);
			for (size_t i = 0; i < chunk; ++i)
			{
				PendingLoad* load = &pending[offset + i];
				tasks[i] = [load]()
					{
						load->Data = FbxResource::LoadBinData(load->BinPath);
					};
			}
			mLoadThreadPool.Dispatch(tasks.data(), chunk);
			mLoadThreadPool.WaitAll();
		}
		DEBUG_LOG(sys::eLogLevel::Log, "FbxResourceManager::PreloadBatchParse: parse phase done.");

		std::vector<std::filesystem::path> srgbPaths, linearPaths;
		for (const auto& load : pending)
		{
			if (!load.Data.Success) continue;
			FbxResource::CollectTexturePaths(load.Data, srgbPaths, linearPaths);
		}

		if (onTotalDiscovered)
		{
			onTotalDiscovered(static_cast<int>(srgbPaths.size() + linearPaths.size()));
		}
		DEBUG_LOG(sys::eLogLevel::Log,
			std::format("FbxResourceManager::PreloadBatchParse: decoding {} texture(s) (srgb={}, linear={})...",
				srgbPaths.size() + linearPaths.size(), srgbPaths.size(), linearPaths.size()));

		auto& texManager = graphics::TextureManager::Get();
		texManager.PreloadBatchDecode(srgbPaths, true);
		texManager.PreloadBatchDecode(linearPaths, false);
		DEBUG_LOG(sys::eLogLevel::Log, "FbxResourceManager::PreloadBatchParse: texture decode done.");

		{
			std::lock_guard<std::mutex> lock(mMutex);
			for (auto& load : pending)
			{
				mPendingParsed.push_back(std::move(load));
			}
		}
	}

	// 貯めた解析結果をすべて解決しクリアする
	void FbxResourceManager::PreloadBatchResolve(const std::function<void()>& onItemLoaded)
	{
		graphics::TextureManager::Get().PreloadBatchResolve(onItemLoaded);

		std::vector<PendingLoad> pending;
		{
			std::lock_guard<std::mutex> lock(mMutex);
			pending = std::move(mPendingParsed);
			mPendingParsed.clear();
		}
		if (pending.empty()) return;

		for (auto& load : pending)
		{
			if (!load.Data.Success) continue;

			auto resource = std::make_unique<FbxResource>();
			if (!resource->CreateFromBinData(load.Data))
			{
				DEBUG_LOG(sys::eLogLevel::Error,
					std::format("FbxResourceManager: Failed to build '{}'", load.BinPath));
				continue;
			}

			{
				std::lock_guard<std::mutex> lock(mMutex);
				mCache.emplace(load.BinPath, std::move(resource));
			}
			if (onItemLoaded) onItemLoaded();
			DEBUG_LOG(sys::eLogLevel::Log,
				std::format("FbxResourceManager: Cached '{}'", load.BinPath));
		}
		DEBUG_LOG(sys::eLogLevel::Log, "FbxResourceManager::PreloadBatchResolve: all done.");
	}
}