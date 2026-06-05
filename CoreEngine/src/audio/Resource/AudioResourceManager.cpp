#include "pch.h"
#include "AudioResourceManager.h"

namespace fs = std::filesystem;

namespace audio
{
	AudioResource* AudioResourceManager::GetResource(const std::string& filePath)
	{
		const std::string key = NormalizePath(filePath);
		if (auto it = mResources.find(key); it != mResources.end())
		{
			return it->second.get();
		}

		auto resource = std::make_unique<AudioResource>();
		if (!resource->LoadFromAud(key))
		{
			DEBUG_LOG(sys::eLogLevel::Error, key);
			return nullptr;
		}


		AudioResource* ptr = resource.get();
		mResources.emplace(key, std::move(resource));
		DEBUG_LOG(sys::eLogLevel::Log,"Load audio resource");
		return ptr;
	}
	void AudioResourceManager::LoadAllFromDirectory(const fs::path& root)
	{
		if (!fs::exists(root) || !fs::is_directory(root))
		{
			DEBUG_LOG(sys::eLogLevel::Warning,"Directory does not exist or is not a directory: ", root.string());
			return;
		}

		for (const auto& entry : fs::recursive_directory_iterator(root))
		{
			if (!entry.is_regular_file()) continue;
			std::string ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return std::tolower(c); });
			if (ext != ".aud") continue;
			GetResource(entry.path().string());
		}
	}
	void AudioResourceManager::UnloadAllFromDirectory(const fs::path& root)
	{
		const std::string prefix = NormalizePath(root);
		const size_t before = mResources.size();
		std::erase_if(mResources,
			[&prefix](const auto& pair) { return pair.first.starts_with(prefix); });
		DEBUG_LOG(sys::eLogLevel::Log, "Unloaded ", before - mResources.size(), " audio resources from directory: ", root.string());
	}
	std::string AudioResourceManager::NormalizePath(const fs::path& filePath)
	{
		return fs::relative(filePath, fs::current_path()).generic_string();
	}
}