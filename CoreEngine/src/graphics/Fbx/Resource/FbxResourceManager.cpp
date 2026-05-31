#include "pch.h"
#include "FbxResourceManager.h"

#include"FbxResource.h"

namespace graphics
{
	FbxResource* FbxResourceManager::Load(const std::string& binPath)
	{
		// キャッシュ済みならそのまま返す
		auto it = mCache.find(binPath);
		if (it != mCache.end()) return it->second.get();

		// 読み込み
		auto resource = std::make_unique<FbxResource>();
		if (!resource->Load(binPath))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				std::format("FbxResourceManager: Failed to load '{}'", binPath));
			return nullptr;
		}

		auto ptr = resource.get();

		mCache.emplace(binPath, std::move(resource));
		DEBUG_LOG(sys::eLogLevel::Log,
			std::format("FbxResourceManager: Cached '{}'", binPath));

		return ptr;

	}

	bool FbxResourceManager::LoadAnm(
		const std::string& binPath,
		const std::string& anmPath,
		const std::string& clipName)
	{
		auto it = mCache.find(binPath);
		if (it == mCache.end())
		{
			return false;
		}

		return it->second->LoadAnm(anmPath, clipName);
	}

	FbxResource* FbxResourceManager::GetResource(const std::string& binPath) const
	{
		auto it = mCache.find(binPath);
		return (it != mCache.end()) ? it->second.get() : nullptr;
	}

	void FbxResourceManager::Unload(const std::string& binPath)
	{
		auto it = mCache.find(binPath);
		if (it == mCache.end()) return;


	}

	void FbxResourceManager::Clear()
	{
		mCache.clear();
		DEBUG_LOG(sys::eLogLevel::Log, "FbxResourceManager: Cleared all cache.");
	}

}