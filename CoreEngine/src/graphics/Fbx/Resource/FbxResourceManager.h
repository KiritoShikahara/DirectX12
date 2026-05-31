#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <Utility/Export/Export.h>

namespace graphics
{
	class FbxResource;

	class ENGINE_API FbxResourceManager : public utility::Singleton<FbxResourceManager>
	{
		SINGLETON_CLASS(FbxResourceManager);
	public:
		SINGLETON_ACCESSOR(FbxResourceManager);

		FbxResource* Load(
			const std::string& binPath);

		bool LoadAnm(
			const std::string& binPath,
			const std::string& anmPath,
			const std::string& clipName = "");

		FbxResource* GetResource(const std::string& binPath) const;

		void Unload(const std::string& binPath);

		/// <summary>全キャッシュをクリアする</summary>
		void Clear();
	private:
		std::unordered_map<std::string, std::unique_ptr<FbxResource>> mCache;
	};
}


