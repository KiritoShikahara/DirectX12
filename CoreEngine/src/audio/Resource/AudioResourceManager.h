#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include"AudioResource.h"

#include<filesystem>
#include<string>
#include<memory>
#include<unordered_map>

namespace audio
{
	class AudioResourceManager : public utility::Singleton<AudioResourceManager>
	{
		SINGLETON_CLASS(AudioResourceManager);
	public:
		SINGLETON_ACCESSOR(AudioResourceManager);

		AudioResource* GetResource(const std::string& filePath);
		void LoadAllFromDirectory(const fs::path& root);
		void UnloadAllFromDirectory(const fs::path& root);

	private:
		static std::string NormalizePath(const fs::path& filePath);
		std::unordered_map<std::string, std::unique_ptr<AudioResource>> mResources;

	};
}


