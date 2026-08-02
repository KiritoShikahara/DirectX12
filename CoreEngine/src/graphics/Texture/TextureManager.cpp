#include"pch.h"
#include "TextureManager.h"
#include"Texture.h"

#include<functional>

namespace graphics
{

	Texture* TextureManager::GetOrLoad(const std::filesystem::path& FilePath, bool isSRGB)
	{
		const std::string key = MakeCacheKey(FilePath, isSRGB);

		// 検索
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto it = mResources.find(key);
			if (it != mResources.end())
			{
				return it->second.get();
			}
		}

		// ロード
		auto newTexture = std::make_unique<Texture>();
		if (!newTexture->Create(FilePath, isSRGB))
		{
			return nullptr;
		}

		// 登録
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto it = mResources.emplace(key, std::move(newTexture)).first;
			return it->second.get();
		}
	}

	void TextureManager::Clear()
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mResources.clear();
	}

	// 同じパスでも色空間の解釈(isSRGB)が異なれば別テクスチャとして扱う。
	std::string TextureManager::MakeCacheKey(const std::filesystem::path& FilePath, bool isSRGB)
	{
		std::string key = std::filesystem::absolute(FilePath).generic_string();
		if (isSRGB)
		{
			key += "|srgb";
		}
		return key;
	}


	void TextureManager::PreloadBatchDecode(const std::vector<std::filesystem::path>& FilePaths, bool isSRGB)
	{
		if (FilePaths.empty()) return;

		if (!mLoadThreadPoolStarted)
		{
			mLoadThreadPool.Initialize();
			mLoadThreadPoolStarted = true;
		}

		// 未キャッシュのパスだけを対象にする
		std::vector<PendingItem> pending;
		pending.reserve(FilePaths.size());
		{
			std::lock_guard<std::mutex> lock(mMutex);
			for (const auto& path : FilePaths)
			{
				std::string key = MakeCacheKey(path, isSRGB);
				if (mResources.find(key) != mResources.end()) continue; // 既にロード済み
				pending.push_back({ path, std::move(key), isSRGB, {} });
			}
		}
		if (pending.empty()) return;

		// CPU側のデコードだけをワーカースレッドへ分配する
		const size_t workerCount = std::max<size_t>(mLoadThreadPool.WorkerCount(), 1);
		std::vector<std::function<void()>> tasks(workerCount);

		for (size_t offset = 0; offset < pending.size(); offset += workerCount)
		{
			const size_t chunk = std::min(workerCount, pending.size() - offset);
			for (size_t i = 0; i < chunk; ++i)
			{
				PendingItem* load = &pending[offset + i];
				tasks[i] = [load, isSRGB]()
					{
						load->Data = Texture::LoadImageData(load->Path, isSRGB);
					};
			}
			mLoadThreadPool.Dispatch(tasks.data(), chunk);
			mLoadThreadPool.WaitAll();
		}

		{
			std::lock_guard<std::mutex> lock(mMutex);
			for (auto& item : pending)
			{
				mPendingParsed.push_back(std::move(item));
			}
		}
	}

	void TextureManager::PreloadBatchResolve(const std::function<void()>& onItemLoaded)
	{
		std::vector<PendingItem> pending;
		{
			std::lock_guard<std::mutex> lock(mMutex);
			pending = std::move(mPendingParsed);
			mPendingParsed.clear();
		}

		for (auto& load : pending)
		{
			if (!load.Data.Success) continue;

			auto texture = std::make_unique<Texture>();
			if (!texture->CreateFromImageData(load.Path, load.IsSRGB, load.Data)) continue;

			{
				std::lock_guard<std::mutex> lock(mMutex);
				mResources.emplace(load.Key, std::move(texture));
			}
			if (onItemLoaded) onItemLoaded();
		}
	}
}