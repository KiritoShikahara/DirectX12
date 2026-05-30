#include"pch.h"
#include "ModelResourceManager.h"
#include"ModelResouce.h"

namespace graphics
{
	std::shared_ptr<ModelResource> ModelResourceManager::Load(const std::string& binPath)
	{
		auto it = mCache.find(binPath);
		if (it != mCache.end()) return it->second;

		auto resource = std::make_shared<ModelResource>();
		if (!resource->Load(binPath))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				std::format("ModelResourceManager: Failed to load '{}'", binPath));
			return nullptr;
		}

		mCache.emplace(binPath, resource);
		return resource;
	}

	bool ModelResourceManager::AppendAnimation(const std::string& binPath,
		const std::string& anmPath,
		const std::string& overrideName)
	{
		auto it = mCache.find(binPath);
		if (it == mCache.end())
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				std::format("ModelResourceManager::AppendAnimation: "
					"先に Load() を呼んでください: '{}'", binPath));
			return false;
		}
		return it->second->AppendAnimation(anmPath, overrideName);
	}

	std::shared_ptr<ModelResource> ModelResourceManager::GetResource(const std::string& binPath) const
	{
		auto it = mCache.find(binPath);
		return (it != mCache.end()) ? it->second : nullptr;
	}

	void ModelResourceManager::Unload(const std::string& binPath)
	{
		auto it = mCache.find(binPath);
		if (it == mCache.end()) return;

		if (it->second.use_count() <= 1)
		{
			mCache.erase(it);
			DEBUG_LOG(sys::eLogLevel::Log,
				std::format("ModelResourceManager: Unloaded '{}'", binPath));
		}
	}

	void ModelResourceManager::Clear()
	{
		mCache.clear();
	}
}